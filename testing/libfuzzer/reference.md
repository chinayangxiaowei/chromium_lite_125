# libFuzzer Integration Reference

## Additional Sanitizer Configuration

### MSan

Memory Sanitizer (MSan) in Chromium only supports Ubuntu Precise/Trusty and not
Rodete.
Thus, our [reproduce tool] cannot reproduce bugs found using MSan.
You can try to reproduce them manually by using [these instructions] on how to
run MSan-instrumented code in docker.

### UBSan

By default, UBSan does not crash when undefined behavior is detected.
To make it crash, the following option needs to be set in environment:
```bash
UBSAN_OPTIONS=halt_on_error=1 ./fuzzer <corpus_directory_or_single_testcase_path>
```
Other useful options are (also used by ClusterFuzz):
```bash
UBSAN_OPTIONS=symbolize=1:halt_on_error=1:print_stacktrace=1 ./fuzzer <corpus_directory_or_single_testcase_path>
```

## Supported Platforms and Configurations

### Builder configurations

The exact GN arguments that are used on our builders can be generated by running
(from Chromium's `src` directory):

| Builder | Description |
|---------|-------------|
|Linux ASan | `tools/mb/mb.py gen -m chromium.fuzz -b 'Libfuzzer Upload Linux ASan' out/libfuzzer` |
|Linux ASan (x86) | `tools/mb/mb.py gen -m chromium.fuzz -b 'Libfuzzer Upload Linux32 ASan' out/libfuzzer` |
|Linux ASan Debug | `tools/mb/mb.py gen -m chromium.fuzz -b 'Libfuzzer Upload Linux ASan Debug' out/libfuzzer` |
|Linux MSan[*](#MSan) | `tools/mb/mb.py gen -m chromium.fuzz -b 'Libfuzzer Upload Linux MSan' out/libfuzzer` |
|Linux UBSan[*](#UBSan)| `tools/mb/mb.py gen -m chromium.fuzz -b 'Libfuzzer Upload Linux UBSan' out/libfuzzer` |
|Chrome OS ASan | `tools/mb/mb.py gen -m chromium.fuzz -b 'Libfuzzer Upload Chrome OS ASan' out/libfuzzer` |
|Mac ASan | `tools/mb/mb.py gen -m chromium.fuzz -b 'Libfuzzer Upload Mac ASan' out/libfuzzer` |
|Windows ASan | `python tools\mb\mb.py gen -m chromium.fuzz -b "Libfuzzer Upload Windows ASan" out\libfuzzer` |
|Linux ASan V8 ARM Simulator[*](#ARM-and-ARM64)| `tools/mb/mb.py gen -m chromium.fuzz -b 'Libfuzzer Upload Linux32 V8-ARM ASan' out/libfuzzer` |
|Linux ASan V8 ARM64 Simulator[*](#ARM-and-ARM64)| `tools/mb/mb.py gen -m chromium.fuzz -b 'Libfuzzer Upload Linux V8-ARM64 ASan' out/libfuzzer` |
|Linux ASan Debug V8 ARM Simulator[*](#ARM-and-ARM64)| `tools/mb/mb.py gen -m chromium.fuzz -b 'Libfuzzer Upload Linux32 V8-ARM ASan Debug' out/libfuzzer` |
|Linux ASan Debug V8 ARM64 Simulator[*](#ARM-and-ARM64)| `tools/mb/mb.py gen -m chromium.fuzz -b 'Libfuzzer Upload Linux V8-ARM64 ASan Debug' out/libfuzzer` |


### Linux
Linux is fully supported by libFuzzer and ClusterFuzz with following sanitizer
configurations:

| GN Argument | Description |
|--------------|----|
| is_asan=true | enables [Address Sanitizer] to catch problems like buffer overruns. |
| is_msan=true | enables [Memory Sanitizer] to catch problems like uninitialized reads. \[[*](#MSan)\] |
| is_ubsan_security=true | enables [Undefined Behavior Sanitizer] to catch undefined behavior like integer overflow. \[[*](#UBSan)\] |

Configuration example:

```bash
# With address sanitizer
gn gen out/libfuzzer '--args=use_libfuzzer=true is_asan=true' --check
```

### Linux x86 (32-bit)
Fuzzing targets built for x86 can discover bugs that are not found by x64
builds. Linux x86 is supported by libFuzzer with `is_asan` configuration.

Configuration example:

```bash
gn gen out/libfuzzer --args="use_libfuzzer=true is_asan=true host_cpu=\"x86\" target_cpu=\"x86\"" --check
```

### Chrome OS
Chrome OS is supported by libFuzzer with `is_asan` configuration.

Configuration example:

```bash
gn gen out/libfuzzer '--args=use_libfuzzer=true is_asan=true target_os="chromeos"' --check
```

To do a Chrome OS build on Linux (not just for libFuzzer), your `.gclient` file
must be configured appropriately, see the [Chrome OS build docs] for more
details.

### Mac

Mac is supported by libFuzzer with `is_asan` configuration.

Configuration example:

```bash
gn gen out/libfuzzer '--args=use_libfuzzer=true is_asan=true mac_deployment_target="10.7"' --check
```

### Windows

Windows is supported by libFuzzer with `is_asan` configuration.

Configuration example:

```bash
gn gen out/libfuzzer "--args=use_libfuzzer=true is_asan=true is_debug=false is_component_build=false" --check
```

On Windows you must use `is_component_build=false` as libFuzzer does not support
component builds on Windows. If you are using `is_asan=true` then you must use
`is_debug=false` as ASan does not support debug builds on Windows.
You may also want to consider using `symbol_level=1` which will reduce build
size by reducing symbol level to the level necessary for libFuzzer (useful
if building many fuzz targets).

### ARM and ARM64

The V8 ARM and ARM64 simulators are supported by libFuzzer with `is_asan`
configuration. Note that there is nothing special about these builds for non-V8
fuzz targets.

ARM configuration example:


```bash
gn gen out/libfuzzer --args="use_libfuzzer=true is_asan=true host_cpu=\"x86\" target_cpu=\"x86\" v8_target_cpu=\"arm\"" --check
```

ARM64 configuration example:

```bash
gn gen out/libfuzzer --args="use_libfuzzer=true is_asan=true target_cpu=\"x64\" v8_target_cpu=\"arm64\"" --check
```

## fuzzer_test GN Template

Use `fuzzer_test` to define libFuzzer targets:

```
fuzzer_test("my_fuzzer") {
  ...
}
```

Following arguments are supported:

| Argument | Description |
|----------|-------------|
| `sources` | **required** list of fuzzer test source files |
| `deps` | fuzzer dependencies |
| `additional_configs` | additional GN configurations to be used for compilation |
| `dict` | a dictionary file for the fuzzer |
| `libfuzzer_options` | runtime options file for the fuzzer. See [Fuzzer Runtime Options](#Fuzzer-Runtime-Options) |
| `seed_corpus` | single directory containing test inputs, parsed recursively |
| `seed_corpuses` | multiple directories with the same purpose as `seed_corpus` |
| `libs` | additional libraries to link. Same as [libs] for gn targets. |


## Fuzzer Runtime Options

There are many different runtime options supported by libFuzzer. Options
are passed as command line arguments:

```
./fuzzer [-flag1=val1 [-flag2=val2 ...] ] [dir1 [dir2 ...] ]
```

Most common flags are:

| Flag | Description |
|------|-------------|
| max_len | Maximum length of test input. |
| timeout | Timeout of seconds. Units slower than this value will be reported as bugs. |
| rss_limit_mb | Memory usage limit in Mb, default 2048. Some Chrome targets, such as Blink, require more than the default to initialize. |

Full list of options can be found at [libFuzzer options] page and by running
the binary with `-help=1`.

To specify these options for ClusterFuzz, list all parameters in
`libfuzzer_options` target attribute:

```
fuzzer_test("my_fuzzer") {
  ...
  libfuzzer_options = [
    # Suppress stdout and stderr output (not recommended, as it may silence useful info).
    "close_fd_mask=3",
  ]
}
```

[libFuzzer options]: http://llvm.org/docs/LibFuzzer.html#options
[Address Sanitizer]: http://clang.llvm.org/docs/AddressSanitizer.html
[Memory Sanitizer]: http://clang.llvm.org/docs/MemorySanitizer.html
[Undefined Behavior Sanitizer]: http://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html
[reproduce tool]: https://github.com/google/clusterfuzz-tools
[these instructions]: https://www.chromium.org/developers/testing/memorysanitizer#TOC-Running-on-other-distros-using-Docker
[Chrome OS build docs]: https://chromium.googlesource.com/chromium/src/+/HEAD/docs/chromeos_build_instructions.md#updating-your-gclient-config
[libs]: https://gn.googlesource.com/gn/+/master/docs/reference.md#libs
