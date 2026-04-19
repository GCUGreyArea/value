# Values Library

This repository contains a small C++17 library with two main parts:

- `ValueMap` and `ValueVector`, lightweight containers built around
  `std::variant`
- `FlgParser`, a configurable parser for `.flg` files containing leading
  key-value metadata followed by tabular CSV-style data

## Project Layout

```text
value/
├── BUILD.bazel
├── MODULE.bazel
├── lib/
│   ├── include/
│   │   ├── Parser.hpp
│   │   ├── ParserConfig.hpp
│   │   └── Values.hpp
│   └── src/
│       ├── Parser.cpp
│       ├── ParserConfig.cpp
│       ├── parser.l
│       └── parser.y
├── test/
│   ├── resources/
│   │   └── input.flg
│   ├── testParser.cpp
│   └── testValues.cpp
├── examples/
│   ├── custom_validators.cpp
│   ├── example.cpp
│   └── parser_examples.cpp
└── Makefile
```

## Build

The primary build uses Bazel.

```bash
bazel build //:example //:parser_examples

```

Run the unit suite with:

```bash
bazel test //:unit_tests
```

The generated parser path still requires both `flex` and `bison` to be
installed because Bazel generates the lexer and parser sources at build time.

To include the external `plmnid` validator in the example binaries at build
time:

```bash
bazel build --define enable_custom_validators=true //:example //:parser_examples
```

The repository still includes a `Makefile` for local compatibility.

## Library Overview

### Value Types

`Value` is a `std::variant` over:

- `int`
- `long`
- `std::string`
- `std::uint8_t`
- `std::uint16_t`
- `std::uint32_t`
- `std::uint64_t`
- `bool`
- `double`
- `float`

### Containers

- `ValueMap` stores named values with typed retrieval
- `ValueVector` stores ordered values with typed retrieval and raw access via
  `at()`

### Flexible LAyout Grammer (FLG) Parser

`FlgParser` supports:

- key-value pairs at the top of the file
- a single header row immediately after the metadata block
- configurable delimiter selection
- configurable column typing by index or by column name
- configurable key-value counts via `setExpectedKeyValuePairs()`
- required headings with warning or error severity
- field-specific string validators
- structured parse diagnostics with line and column numbers

The runtime parser is generated from `lib/src/parser.l` and
`lib/src/parser.y`, with
[lib/src/ParserDriver.cpp](/home/barry/workspace/value/lib/src/ParserDriver.cpp:1)
owning FLG-specific configuration, heuristics, and diagnostics.

## Example Formats

```text
KEYONE,1.87
KEYTWO,NAME_ONE
KEYTHREE,VALUE_NOW
HEADING1,HEADING2,HEADING3,HEADING4
144.6,223-20,-75,false
22.5,213-70,-55,true
```

## Usage

```cpp
#include "Parser.hpp"
#include <fstream>

FlgParser parser;
parser.setExpectedKeyValuePairs(5);
parser.setDelimiter(',');
parser.setColumnType(0, "HEADING1", ValueType::FLOAT);
parser.setColumnType(1, "HEADING2", ValueType::STRING);
parser.setColumnType(3, "HEADING4", ValueType::BOOL);
parser.setRequiredHeading("HEADING4", DiagnosticSeverity::ERROR);
parser.setFieldValidator("STAMP", "datetime_utc");

std::ifstream input("input.flg");
if (input && parser.deserialise(input)) {
    const auto& row = parser.getRow(0);
    float value = row.get<float>(0);
    std::string label = row.get<std::string>(1);
    bool enabled = row.get<bool>(3);
} else if (const ParseError* error = parser.getLastError()) {
    std::cerr << "Parse error at line " << error->line
              << ", column " << error->column
              << ": " << error->message << '\n';
}

for (const auto& warning : parser.getWarnings()) {
    std::cerr << "Warning at line " << warning.line
              << ", column " << warning.column
              << ": " << warning.message << '\n';
}
```

To write the current parser state back out in FLG format:

```cpp
std::ofstream output("roundtrip.flg");
if (!output || !parser.serialise(output)) {
    std::cerr << "Failed to serialise parser contents\n";
}
```

## Example CLI

The `example` binary now reads parser settings from JSON and takes the input
file path on the command line:

```bash
./example examples/example_config.json input.flg
```

For semicolon-delimited input:

```bash
./example examples/example_config_semicolon.json data_semicolon.flg
```

Sample config:

```json
{
  "expected_key_value_pairs": 5,
  "delimiter": ",",
  "field_requirements": {
    "HEADING4": "error",
    "STAMP": "optional"
  },
  "validators": {
    "STAMP": "datetime_utc"
  },
  "columns": [
    { "index": 0, "name": "HEADING1", "type": "FLOAT" },
    { "index": 1, "name": "HEADING2", "type": "STRING" },
    { "index": 2, "name": "HEADING3", "type": "INT" },
    { "index": 3, "name": "HEADING4", "type": "BOOL" }
  ]
}
```

`field_requirements` accepts `error`, `warning`, or `optional`. `validators`
maps field names to registered validator names.

Built-in validators:

- `datetime_utc`: validates `YYYY-MM-DDTHH:MM:SSZ`

External validators can be registered from a separate compilation unit with
`registerValidator()`. The repository includes an example `plmnid` validator in
[custom_validators.cpp](/home/barry/workspace/value/examples/custom_validators.cpp),
which validates a `PLMNID` string as a 3-digit MCC followed by a 2- or 3-digit
MNC.

Semicolon-delimited sample config:

```json
{
  "expected_key_value_pairs": 2,
  "delimiter": ";",
  "columns": [
    { "index": 0, "name": "ID", "type": "UINT32" },
    { "index": 1, "name": "NAME", "type": "STRING" },
    { "index": 2, "name": "ACTIVE", "type": "BOOL" },
    { "index": 3, "name": "SCORE", "type": "FLOAT" }
  ]
}
```

## Additional Documentation

- [PARSER_USAGE.md](PARSER_USAGE.md)
- [PARSER_ARCHITECTURE.md](PARSER_ARCHITECTURE.md)
- [PARSER_IMPLEMENTATION.md](PARSER_IMPLEMENTATION.md)
