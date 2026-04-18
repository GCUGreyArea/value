# FLG Parser Usage

## Overview

`FlgParser` parses `.flg` files with:

- leading key-value metadata
- one header row defining columns
- zero or more data rows

The active runtime implementation is the manual parser in
`lib/src/Parser.cpp`. The Flex/Bison sources remain in `lib/src/parser.l` and
`lib/src/parser.y` as reference grammar assets and are not used by the default
build.

## Supported Input Shapes

### Input.flg-style direct header row

```text
KEYONE,1.87
KEYTWO,NAME_ONE
KEYTHREE,VALUE_NOW
HEADING1,HEADING2,HEADING3,HEADING4
144.6,223-20,-75,false
22.5,213-70,-55,true
```

This is the intended format: key-value metadata first, then a single header row,
then data rows with the same number of columns.

### Ambiguous headers

If your header row has zero or one delimiters, or you want to force the parser
to stop reading metadata after a known number of rows, call
`setExpectedKeyValuePairs()`.

## API

### Include

```cpp
#include "Parser.hpp"
```

### Basic parsing

```cpp
FlgParser parser;
parser.setExpectedKeyValuePairs(5);  // Optional, but useful when the count is known

if (!parser.parseFile("input.flg")) {
    return 1;
}
```

### Configure delimiter

The default delimiter is `,`, but you can change it:

```cpp
parser.setDelimiter(';');
```

### Configure parser from JSON

The example CLI uses `nlohmann::json` to configure the parser from a JSON file.
Supported config fields are:

- `expected_key_value_pairs`: integer
- `delimiter`: single-character string
- `columns`: array of `{ "index", "name", "type" }`
- `column_types_by_name`: object mapping column names to type names

Example:

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

Semicolon-delimited example:

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

### Configure column types

By index:

```cpp
parser.setColumnType(0, "ID", ValueType::UINT32);
parser.setColumnType(1, "NAME", ValueType::STRING);
parser.setColumnType(2, "ACTIVE", ValueType::BOOL);
```

By name:

```cpp
parser.setColumnTypeByName("SCORE", ValueType::FLOAT);
parser.setColumnTypeByName("ACTIVE", ValueType::BOOL);
```

### Access results

```cpp
const auto& kv = parser.getKeyValuePairs();
const auto& columns = parser.getColumnDefinitions();
const auto& row = parser.getRow(0);

std::uint32_t id = row.get<std::uint32_t>(0);
std::string name = row.get<std::string>(1);
bool active = row.get<bool>(2);
```

### Mixed-type display

Use `ValueVector::at()` with `valueToString()` when iterating across mixed
columns:

```cpp
for (size_t i = 0; i < row.size(); ++i) {
    std::cout << valueToString(row.at(i)) << '\n';
}
```

### Empty cells

Empty cells are preserved when they appear at the start, middle, or end of a
row.

- string columns store missing cells as `""`
- typed columns also preserve missing cells, but failed conversion falls back to
  `std::string("")`

That means an empty value in an `INT` column is retained, but `row.get<int>()`
will throw on that cell because the stored variant alternative is
`std::string`.

### Reuse parser configuration

`parseFile()` and `parseString()` replace previously parsed rows, columns, and
metadata, but preserve configured column types. Call `clear()` if you want to
reset both parsed data and configuration.

### Parse diagnostics

Failed parses populate `getErrors()` and `getLastError()`:

```cpp
if (!parser.parseFile("input.flg")) {
    if (const ParseError* error = parser.getLastError()) {
        std::cerr << "Parse error at line " << error->line
                  << ", column " << error->column
                  << ": " << error->message << '\n';
    }
}
```

Typical diagnostics include:

- unexpected type for a configured column
- too many delimiters in a key-value line
- too many delimiters in a data row
- too few fields in a data row
- invalid structure when the file does not match the expected `input.flg` shape

## Supported Types

`ValueType` supports:

- `INT`
- `LONG`
- `STRING`
- `UINT8`
- `UINT16`
- `UINT32`
- `UINT64`
- `BOOL`
- `DOUBLE`
- `FLOAT`

## Conversion Rules

- key-value metadata is inferred as `double`, `long`, or `std::string`
- typed data columns are converted according to configured `ValueType`
- non-empty invalid typed values produce parse errors with line and column
- booleans accept `true`/`false` case-insensitively and numeric `0`/non-zero
- empty cells are preserved as `std::string("")`

## Build And Run

```bash
make
./unit_tests
./example
./parser_examples
```

## Error Behavior

- `parseFile()` returns `false` if the file cannot be opened
- `parseString()` returns `false` for invalid structure, including marker-style
  input that does not match the `input.flg` shape
- `getLastError()` returns the most recent parse diagnostic
- parse diagnostics are 1-based for both line and column numbers
- `getRow()` throws `std::out_of_range` for invalid row access
- `ValueVector::get<T>()` throws for bounds or type mismatches
- `ValueMap::get<T>()` throws `missing_key` or `wrong_type`
