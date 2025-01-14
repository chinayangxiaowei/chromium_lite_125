#!/usr/bin/env python
# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Code generator for proto descriptors used for on-device model execution.

This script generates a C++ source file containing the proto descriptors.
"""
from __future__ import annotations

import dataclasses
import functools
from io import StringIO
import optparse
import os
import collections
import re
import sys

_HERE_PATH = os.path.dirname(__file__)
_SRC_PATH = os.path.normpath(os.path.join(_HERE_PATH, '..', '..', '..'))
sys.path.insert(0, os.path.join(_SRC_PATH, 'third_party', 'protobuf',
                                'python'))

from google.protobuf import descriptor_pb2


class Error(Exception):
    pass


class Type:
    """Aliases for FieldDescriptorProto::Type(s)."""
    DOUBLE = 1
    FLOAT = 2
    INT64 = 3
    UINT64 = 4
    INT32 = 5
    FIXED64 = 6
    FIXED32 = 7
    BOOL = 8
    STRING = 9
    GROUP = 10
    MESSAGE = 11
    BYTES = 12
    UINT32 = 13
    ENUM = 14
    SFIXED32 = 15
    SFIXED64 = 16
    SINT32 = 17
    SINT64 = 18


@dataclasses.dataclass(frozen=True)
class Message:
    desc: descriptor_pb2.DescriptorProto
    package: str
    parent_name: str | None

    @functools.cached_property
    def type_name(self) -> str:
        if (self.parent_name):
            return f'{self.package}.{self.parent_name}.{self.desc.name}'
        return f'{self.package}.{self.desc.name}'

    @functools.cached_property
    def cpp_name(self) -> str:
        package = self.package.replace('.', '::')
        if (self.parent_name):
            return f'{package}::{self.parent_name}_{self.desc.name}'
        return f'{package}::{self.desc.name}'

    @functools.cached_property
    def fields(self):
        return tuple(Field(fdesc) for fdesc in self.desc.field)


@dataclasses.dataclass(frozen=True)
class Field:
    desc: descriptor_pb2.FieldDescriptorProto

    @property
    def tag_number(self):
        return self.desc.number

    @property
    def name(self):
        return self.desc.name

    @property
    def type(self):
        return self.desc.type

    @property
    def is_repeated(self):
        return self.desc.label == 3


def GenerateProtoDescriptors(out, includes, messages: list[Message]):
    """Generate the on_device_model_execution_proto_descriptors.cc content."""

    out.write(
        '// DO NOT MODIFY. GENERATED BY gen_on_device_proto_descriptors.py\n')
    out.write('\n')

    out.write(
        '#include "components/optimization_guide/core/model_execution/on_device_model_execution_proto_descriptors.h"'  # pylint: disable=line-too-long
    )
    out.write('\n\n')

    for include in includes:
        out.write(f'#include {include}\n')
    out.write('\n')

    out.write('namespace optimization_guide {\n')
    out.write('\n')
    out.write('namespace {\n')
    _GetProtoValue.GenPrivate(out, messages)
    _GetProtoRepeated.GenPrivate(out, messages)
    _SetProtoValue.GenPrivate(out, messages)
    out.write('}  // namespace\n\n')
    _GetProtoValue.GenPublic(out)
    _GetProtoRepeated.GenPublic(out)
    _SetProtoValue.GenPublic(out)
    _NestedMessageIteratorGet.GenPublic(out, messages)
    out.write("""\
      NestedMessageIterator::NestedMessageIterator(
            const google::protobuf::MessageLite* parent,
            int32_t tag_number,
            int32_t field_size,
            int32_t offset) :
          parent_(parent),
          tag_number_(tag_number),
          field_size_(field_size),
          offset_(offset) {}
      """)
    out.write('}  // namespace optimization_guide\n')
    out.write('\n')


class _GetProtoValue:
    """Namespace class for GetProtoValue method builders."""

    @classmethod
    def GenPublic(cls, out):
        out.write("""
          std::optional<proto::Value> GetProtoValue(
              const google::protobuf::MessageLite& msg,
              const proto::ProtoField& proto_field) {
            return GetProtoValue(msg, proto_field, /*index=*/0);
          }
        """)

    @classmethod
    def GenPrivate(cls, out, messages: list[Message]):
        out.write("""
          std::optional<proto::Value> GetProtoValue(
              const google::protobuf::MessageLite& msg,
              const proto::ProtoField& proto_field, int32_t index) {
            if (index >= proto_field.proto_descriptors_size()) {
              return std::nullopt;
            }
            int32_t tag_number =
                proto_field.proto_descriptors(index).tag_number();
        """)

        for msg in messages:
            cls._IfMsg(out, msg)
        out.write('return std::nullopt;\n')
        out.write('}\n\n')  # End function

    @classmethod
    def _IfMsg(cls, out, msg: Message):
        if all(field.is_repeated for field in msg.fields):
            # Omit the empty case to avoid unused variable warnings.
            return
        out.write(f'if (msg.GetTypeName() == "{msg.type_name}") {{\n')
        out.write(f'const {msg.cpp_name}& casted_msg = ')
        out.write(f'  static_cast<const {msg.cpp_name}&>(msg);\n')
        out.write('switch (tag_number) {\n')
        for field in msg.fields:
            if field.is_repeated:
                continue
            cls._FieldCase(out, field)
        out.write('}\n')  # End switch
        out.write('}\n\n')  # End if statement

    @classmethod
    def _FieldCase(cls, out, field: Field):
        out.write(f'case {field.tag_number}: {{\n')
        name = f'casted_msg.{field.name}()'
        if field.type == Type.MESSAGE:
            out.write(f'return GetProtoValue({name}, proto_field, index+1);\n')
        else:
            out.write('proto::Value value;\n')
            if field.type in {Type.DOUBLE, Type.FLOAT}:
                out.write(
                    f'value.set_float_value(static_cast<double>({name}));\n')
            elif field.type in {Type.INT64, Type.UINT64}:
                out.write(
                    f'value.set_int64_value(static_cast<int64_t>({name}));\n')
            elif field.type in {Type.INT32, Type.UINT32, Type.ENUM}:
                out.write(
                    f'value.set_int32_value(static_cast<int32_t>({name}));\n')
            elif field.type in {Type.BOOL}:
                out.write(f'value.set_boolean_value({name});\n')
            elif field.type in {Type.STRING}:
                out.write(f'value.set_string_value({name});\n')
            else:
                raise Error()
            out.write('return value;\n')
        out.write('}\n')  # End case


class _NestedMessageIteratorGet:
    """Namespace class for NestedMessageIterator::Get method builders."""

    @classmethod
    def GenPublic(cls, out, messages: list[Message]):
        out.write('const google::protobuf::MessageLite* '
                  'NestedMessageIterator::Get() const {\n')
        for msg in messages:
            cls._IfMsg(out, msg)
        out.write('  NOTREACHED();\n')
        out.write('  return nullptr;\n')
        out.write('}\n')

    @classmethod
    def _IfMsg(cls, out, msg: Message):
        out.write(f'if (parent_->GetTypeName() == "{msg.type_name}") {{\n')
        out.write('switch (tag_number_) {\n')
        for field in msg.fields:
            if field.type == Type.MESSAGE and field.is_repeated:
                cls._FieldCase(out, msg, field)
        out.write('}\n')  # End switch
        out.write('}\n\n')  # End if statement

    @classmethod
    def _FieldCase(cls, out, msg: Message, field: Field):
        cast_msg = f'static_cast<const {msg.cpp_name}*>(parent_)'
        out.write(f'case {field.tag_number}: {{\n')
        out.write(f'return &{cast_msg}->{field.name}(offset_);\n')
        out.write('}\n')  # End case


class _GetProtoRepeated:
    """Namespace class for GetProtoRepeated method builders."""

    @classmethod
    def GenPublic(cls, out):
        out.write("""
          std::optional<NestedMessageIterator> GetProtoRepeated(
              const google::protobuf::MessageLite* msg,
              const proto::ProtoField& proto_field) {
            return GetProtoRepeated(msg, proto_field, /*index=*/0);
          }
          """)

    @classmethod
    def GenPrivate(cls, out, messages: list[Message]):
        out.write("""\
          std::optional<NestedMessageIterator> GetProtoRepeated(
              const google::protobuf::MessageLite* msg,
              const proto::ProtoField& proto_field,
              int32_t index) {
            if (index >= proto_field.proto_descriptors_size()) {
              return std::nullopt;
            }
            int32_t tag_number =
                proto_field.proto_descriptors(index).tag_number();
          """)

        for msg in messages:
            cls._IfMsg(out, msg)
        out.write('return std::nullopt;\n')
        out.write('}\n\n')  # End function

    @classmethod
    def _IfMsg(cls, out, msg: Message):
        out.write(f'if (msg->GetTypeName() == "{msg.type_name}") {{\n')
        out.write('switch (tag_number) {\n')
        for field in msg.fields:
            if field.type == Type.MESSAGE:
                cls._FieldCase(out, msg, field)
        out.write('}\n')  # End switch
        out.write('}\n\n')  # End if statement

    @classmethod
    def _FieldCase(cls, out, msg: Message, field: Field):
        field_expr = f'static_cast<const {msg.cpp_name}*>(msg)->{field.name}()'
        out.write(f'case {field.tag_number}: {{\n')
        if field.is_repeated:
            out.write(f'return NestedMessageIterator('
                      f'msg, tag_number, {field_expr}.size(), 0);\n')
        else:
            out.write(f'return GetProtoRepeated('
                      f'&{field_expr}, proto_field, index+1);\n')
        out.write('}\n')  # End case


class _SetProtoValue:
    """Namespace class for SetProtoValue method builders."""

    @classmethod
    def GenPublic(cls, out):
        out.write("""
      std::optional<proto::Any> SetProtoValue(
          const std::string& proto_name,
          const proto::ProtoField& proto_field,
          const std::string& value) {
        return SetProtoValue(proto_name, proto_field, value, /*index=*/0);
      }
    """)

    @classmethod
    def GenPrivate(cls, out, messages: list[Message]):
        out.write("""
      std::optional<proto::Any> SetProtoValue(
          const std::string& proto_name,
          const proto::ProtoField& proto_field,
          const std::string& value,
          int32_t index) {
        if (index >= proto_field.proto_descriptors_size()) {
          return std::nullopt;
        }
    """)
        for msg in messages:
            cls._IfMsg(out, msg)
        out.write("""
        return std::nullopt;
      }
    """)

    @classmethod
    def _IfMsg(cls, out, msg: Message):
        out.write(f'if (proto_name == "{msg.type_name}") {{\n')
        out.write(
            'switch(proto_field.proto_descriptors(index).tag_number()) {\n')
        for field in msg.fields:
            cls._FieldCase(out, msg, field)
        out.write("""
      default:
        return std::nullopt;\n
      """)
        out.write('}')
        out.write('}\n')  # End if statement

    @classmethod
    def _FieldCase(cls, out, msg: Message, field: Field):
        if field.type == Type.STRING:
            out.write(f'case {field.tag_number}: {{\n')
            out.write('proto::Any any;\n')
            out.write(
                f'any.set_type_url("type.googleapis.com/{msg.type_name}");\n')
            out.write(f'{msg.cpp_name} response_value;\n')
            out.write(f'response_value.set_{field.name}(value);')
            out.write('response_value.SerializeToString(any.mutable_value());')
            out.write('return any;')
            out.write('}\n')


def main(argv):
    parser = optparse.OptionParser()
    parser.add_option('--input_file', action='append', default=[])
    parser.add_option('--output_cc')
    parser.add_option('--include', action='append', default=[])
    options, _ = parser.parse_args(argv)

    input_files = list(options.input_file)
    includes = list(options.include)

    # Write to standard output or file specified by --output_cc.
    out_cc = getattr(sys.stdout, 'buffer', sys.stdout)
    if options.output_cc:
        out_cc = open(options.output_cc, 'wb')

    fds_set = []
    for input_file in input_files:
        fds = descriptor_pb2.FileDescriptorSet()
        with open(input_file, 'rb') as fp:
            fds.ParseFromString(fp.read())
            fds_set.append(fds)

    messages = []
    for fds in fds_set:
        for f in fds.file:
            for m in f.message_type:
                messages.append(
                    Message(desc=m, package=f.package, parent_name=None))
                for nested_type in m.nested_type:
                    messages.append(
                        Message(desc=nested_type,
                                package=f.package,
                                parent_name=m.name))

    out_cc_str = StringIO()
    GenerateProtoDescriptors(out_cc_str, includes, messages)
    out_cc.write(out_cc_str.getvalue().encode('utf-8'))

    if options.output_cc:
        out_cc.close()

    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
