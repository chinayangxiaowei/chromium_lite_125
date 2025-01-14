# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Utilities for invoking recipes"""

import asyncio
import json
import logging
import os
import pathlib
import shutil
import subprocess
import sys
import tempfile

from collections import namedtuple
from rich import markdown
from rich import console

import output_adapter

# Disable some noisy logs.
logging.getLogger('asyncio').setLevel(logging.WARNING)
logging.getLogger('markdown_it').setLevel(logging.WARNING)

_THIS_DIR = pathlib.Path(__file__).resolve().parent
_SRC_DIR = _THIS_DIR.parents[1]

RerunOption = namedtuple('RerunOption', ['prompt', 'properties'])


def check_rdb_auth():
  """Checks that the user is logged in with resultdb."""
  rdb_path = shutil.which('rdb')
  if not rdb_path:
    logging.error("'rdb' binary not found. Is depot_tools not on PATH?")
    return False
  cmd = [rdb_path, 'auth-info']
  try:
    p = subprocess.run(cmd,
                       stdout=subprocess.PIPE,
                       stderr=subprocess.STDOUT,
                       text=True,
                       check=True)
  except subprocess.CalledProcessError:
    logging.error('No rdb auth available:')
    logging.error(p.stdout.strip())
    logging.error("Please run 'rdb auth-login' to authenticate")
    return False
  return True


def get_prompt_resp(rerun_props):
  """Prompts the user for how to continue based on recipe output

  Args:
    rerun_props: A list of namedtuples[str, dict] containing the prompt to show
        and the dict of properties to use if that prompt is selected.
  Returns:
    Dict of properties to use for the next recipe invocation. None or an empty
        dict of properties indicate the recipe should not be reinvoked.
  """
  options = '/'.join(f'({option.prompt[0]}){option.prompt[1:]}'
                     for option in rerun_props)
  prompt = (f'How do you wish to proceed? Please enter {options} to confirm: ')
  resp = input(prompt).strip().lower()

  for option in rerun_props:
    # An empty resp will default to the first option like a --force run
    if option.prompt.lower().startswith(resp):
      return option.properties
  return None


class LegacyRunner:
  """Interface for running the UTR recipe via the legacy `recipes.py run` mode.

  TODO(crbug.com/326904531): Sometime in Q2 2024, a more modernized option for
  running recipes locally should be made available. This file can/should be
  updated to support and utilize that new mode if/when it's available.
  """

  UTR_RECIPE_NAME = 'chromium/universal_test_runner'

  def __init__(self,
               recipes_py,
               builder_props,
               bucket,
               builder,
               swarming_server,
               tests,
               skip_compile,
               skip_test,
               skip_prompts,
               build_dir=None):
    """Constructor for LegacyRunner

    Args:
      recipes_py: pathlib.Path to the root of the recipe bundle
      builder_props: Dict containing the props for the builder to run as.
      bucket: Bucket name of the builder to run as.
      builder: Builder name of the builder to run as.
      swarming_server: Swarming server the builder runs on.
      tests: List of tests to run.
      skip_compile: If True, the UTR will only run the tests.
      skip_test: If True, the UTR will only compile.
      skip_prompts: If True, skip Y/N prompts for warnings.
      build_dir: pathlib.Path to the build dir to build in. Will use the UTR's
          default otherwise if needed.
    """
    self._recipes_py = recipes_py
    self._swarming_server = swarming_server
    self._skip_prompts = skip_prompts
    self._console_printer = console.Console()
    assert self._recipes_py.exists()

    # Add UTR recipe props. Its schema is located at:
    # https://chromium.googlesource.com/chromium/tools/build/+/HEAD/recipes/recipes/chromium/universal_test_runner.proto
    input_props = builder_props.copy()
    input_props['checkout_path'] = str(_SRC_DIR)
    input_props['$recipe_engine/path'] = {'cache_dir': str(_SRC_DIR.parent)}
    input_props['test_names'] = tests
    if build_dir:
      input_props['build_dir'] = str(build_dir.absolute())
    # The recipe will overwrite this property so we have to put it preserve it
    # elsewhere
    if 'recipe' in input_props:
      input_props['builder_recipe'] = input_props['recipe']

    mode = 'RUN_TYPE_COMPILE_AND_RUN'
    assert not (skip_compile and skip_test)
    if skip_compile:
      mode = 'RUN_TYPE_RUN'
    elif skip_test:
      mode = 'RUN_TYPE_COMPILE'
    input_props['run_type'] = mode

    # Need to pretend we're an actual build for various builder look-ups in
    # the recipe.
    input_props['$recipe_engine/buildbucket'] = {
        'build': {
            'builder': {
                # Should be safe to hard-code to 'chromium' even if the current
                # checkout is on a release branch.
                'project': 'chromium',
                'bucket': bucket,
                'builder': builder,
            },
        },
    }
    self._input_props = input_props

  def _run(self, adapter, additional_props=None):
    """Internal implementation of invoking `recipes.py run`.

    Args:
      adapter: A output_adapter.Adapter for parsing recipe output.
      additional_props: Dict containing additional props to pass to the recipe.
    Returns:
      Tuple of
        exit code of the `recipes.py` invocation,
        summary markdown of the `recipes.py` invocation,
        a dict of additional_props the recipe should be re-invoked with
    """
    input_props = self._input_props.copy()
    input_props.update(additional_props or {})
    with tempfile.TemporaryDirectory() as tmp_dir:

      # TODO(crbug.com/41492688): Support both chrome and chromium realms. Just
      # hard-code 'chromium' for now.
      # Put all results in "try" realms. "try" should be writable for most devs,
      # while other realms like "ci" likely aren't. "try" is generally where we
      # confine untested code, so it's the best fit for our results here.
      rdb_realm = 'chromium:try'

      output_path = pathlib.Path(tmp_dir).joinpath('out.json')
      rerun_props_path = pathlib.Path(tmp_dir).joinpath('rerun_props.json')
      input_props['output_properties_file'] = str(rerun_props_path)
      cmd = [
          'rdb',
          'stream',
          '-new',
          '-realm',
          rdb_realm,
          '--',
          self._recipes_py,
          'run',
          '--output-result-json',
          output_path,
          '--properties-file',
          '-',  # '-' means read from stdin
          self.UTR_RECIPE_NAME,
      ]
      env = os.environ.copy()
      # This env var is read by both the cas and swarming recipe modules to
      # determine where to upload/run things.
      env['SWARMING_SERVER'] = f'https://{self._swarming_server}.appspot.com'

      async def exec_recipe():
        proc = await asyncio.create_subprocess_exec(
            cmd[0],
            *cmd[1:],
            limit=1024 * 1024 * 128,  # 128 MiB: there can be massive lines
            env=env,
            stdin=asyncio.subprocess.PIPE,
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.STDOUT)

        proc.stdin.write(json.dumps(input_props).encode('ascii'))
        proc.stdin.write_eof()
        while not proc.stdout.at_eof():
          try:
            line = await proc.stdout.readline()
            adapter.ProcessLine(line.decode('utf-8').strip(os.linesep))
          except ValueError:
            logging.exception('Failed to parse line from the recipe')
        await proc.wait()
        return proc.returncode

      returncode = asyncio.run(exec_recipe())

      # Try to pull out the summary markdown from the recipe run.
      failure_md = ''
      if not output_path.exists():
        logging.error('Recipe output json not found')
      else:
        try:
          with open(output_path) as f:
            output = json.load(f)
          failure_md = output.get('failure', {}).get('humanReason', '')
          # TODO(crbug.com/41492688): Also pull out info about gclient/GN arg
          # mismatches, surface those as a Y/N prompt to the user, and re-run
          # if Y.
        except json.decoder.JSONDecodeError:
          logging.exception('Recipe output is invalid json')

      # If this file exists, the recipe is signalling to us that there's an
      # issue, and that we need to re-run if we're sure we want to proceed.
      # The contents of the file are the properties we should re-run it with.
      rerun_props = []
      if rerun_props_path.exists():
        with open(rerun_props_path) as f:
          raw_json = json.load(f)
          for prompt in raw_json:
            rerun_props.append(
                RerunOption(prompt=prompt[0], properties=prompt[1]))

      return returncode, failure_md, rerun_props

  def run_recipe(self, filter_stdout=True):
    """Runs the UTR recipe with the settings defined on the CLI.

    Args:
      filter_stdout: If True, filters noisy log output from the recipe.
    Returns:
      Tuple of (exit code, error message) of the `recipes.py` invocation.
    """
    props_to_use = None
    if filter_stdout:
      adapter = output_adapter.LegacyOutputAdapter()
    else:
      adapter = output_adapter.PassthroughAdapter()
    # We might need to run the recipe a handful of times before we receive a
    # final result. Put a cap on the amount of re-runs though, just in case.
    for _ in range(10):
      exit_code, failure_md, rerun_prop_options = self._run(
          adapter, props_to_use)
      # For in-line code snippets in markdown, style them as python. This
      # seems the least weird-looking.
      pretty_md = markdown.Markdown(failure_md, inline_code_lexer='python')
      if not rerun_prop_options:
        if exit_code:
          # Use the markdown printer from "rich" to better format the text in
          # a terminal.
          md = pretty_md if pretty_md else 'Unknown error'
          self._console_printer.print(md, style='red')
        else:
          logging.info('[green]Success![/]')

        results_link = adapter.GetTestResultsLink()
        if results_link:
          logging.info('')
          logging.info('For futher information, see the full test results at:')
          logging.info(results_link)
        return exit_code, 'Build/test failure' if exit_code else None
      logging.warning('')
      self._console_printer.print(pretty_md)
      logging.warning('')
      if not self._skip_prompts:
        props_to_use = get_prompt_resp(rerun_prop_options)
      else:
        logging.warning(
            '[yellow]Proceeding despite the recipe warning due to the presence '
            'of "--force".[/]')
        if len(rerun_prop_options) < 1 or len(rerun_prop_options[0]) < 2:
          return 1, 'Received bad run options from the recipe'
        # Properties of the first option is the default path
        props_to_use = rerun_prop_options[0].properties
      if not props_to_use:
        return exit_code, 'User-aborted due to warning'
    return 1, 'Exceeded too many recipe re-runs'
