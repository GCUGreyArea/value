# Values Library

This repository contains a small C++17 library with two main parts:

- `ValueMap` and `ValueVector`, lightweight containers built around
  `std::variant`
- `FlgParser`, a configurable parser for `.flg` files containing leading
  key-value metadata followed by tabular CSV-style data

## Project Layout

```text
value/
├── lib/
│   ├── include/
│   │   ├── Parser.hpp
│   │   └── Values.hpp
│   └── src/
│       ├── Parser.cpp
│       ├── parser.l
│       └── parser.y
├── test/
│   ├── resources/
│   │   └── input.flg
│   ├── testParser.cpp
│   └── testValues.cpp
├── examples/
│   ├── example.cpp
│   └── parser_examples.cpp
└── Makefile
```

## Build

The checked-in build uses the provided `Makefile`.

```bash
make
```

This builds:

- `unit_tests`
- `example`
- `parser_examples`

Clean generated outputs with:

```bash
make clean
```

## Test

Run the unit suite with:

```bash
./unit_tests
```

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

### Parser

`FlgParser` supports:

- key-value pairs at the top of the file
- a single header row immediately after the metadata block
- configurable delimiter selection
- configurable column typing by index or by column name
- configurable key-value counts via `setExpectedKeyValuePairs()`
- structured parse errors with line and column numbers

The current runtime parser is implemented in
[lib/src/Parser.cpp](/home/barry/workspace/value/lib/src/Parser.cpp:1).
The Flex/Bison sources in `lib/src/parser.l` and `lib/src/parser.y` are kept as
reference grammar artifacts and are not part of the default build.

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

FlgParser parser;
parser.setExpectedKeyValuePairs(5);
parser.setDelimiter(',');
parser.setColumnType(0, "HEADING1", ValueType::FLOAT);
parser.setColumnType(1, "HEADING2", ValueType::STRING);
parser.setColumnType(3, "HEADING4", ValueType::BOOL);

if (parser.parseFile("input.flg")) {
    const auto& row = parser.getRow(0);
    float value = row.get<float>(0);
    std::string label = row.get<std::string>(1);
    bool enabled = row.get<bool>(3);
} else if (const ParseError* error = parser.getLastError()) {
    std::cerr << "Parse error at line " << error->line
              << ", column " << error->column
              << ": " << error->message << '\n';
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
  "columns": [
    { "index": 0, "name": "HEADING1", "type": "FLOAT" },
    { "index": 1, "name": "HEADING2", "type": "STRING" },
    { "index": 2, "name": "HEADING3", "type": "INT" },
    { "index": 3, "name": "HEADING4", "type": "BOOL" }
  ]
}
```

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
