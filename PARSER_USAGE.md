# FLG Parser Usage

## Overview

`FlgParser` parses `.flg` files with:

- leading key-value metadata
- one header row defining columns
- zero or more data rows

The active runtime implementation uses:

- `lib/src/parser.l`
- `lib/src/parser.y`
- `lib/src/ParserDriver.cpp`

`FlgParser::deserialise()` feeds the input stream into the generated parser,
while `ParserDriver` preserves the FLG-specific metadata/header heuristics,
typed row conversion, and diagnostics policy.

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
#include <fstream>

FlgParser parser;
parser.setExpectedKeyValuePairs(5);  // Optional, but useful when the count is known

std::ifstream input("input.flg");
if (!input || !parser.deserialise(input)) {
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
- `field_requirements`: object mapping field names to `error`, `warning`, or
  `optional`
- `validators`: object mapping field names to validator names such as
  `datetime_utc`

Example:

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

### Configure required headings

Headings are optional by default. Mark headings as required when they must be
present in the header row:

```cpp
parser.setRequiredHeading("ID", DiagnosticSeverity::ERROR);
parser.setRequiredHeading("COMMENT", DiagnosticSeverity::WARNING);
```

Use `setOptionalHeading()` to clear a previous requirement:

```cpp
parser.setOptionalHeading("COMMENT");
```

### Configure field validators

Bind a field to a registered validator name:

```cpp
parser.setFieldValidator("STAMP", "datetime_utc");
```

Validator functions use this signature:

```cpp
using StringFieldValidator =
    std::function<bool(const std::string& value, std::string& errorMessage)>;
```

Return `true` for valid values. Return `false` and populate `errorMessage` to
emit a parse error for that field.

Register validators by name so they can be referenced from code or JSON:

```cpp
registerValidator("datetime_utc", createDateTimeUtcValidator());
```

The built-in registry already includes `datetime_utc`, which checks the UTC
timestamp format `YYYY-MM-DDTHH:MM:SSZ`, for example
`2026-04-19T12:34:56Z`.

External validators can be added from a separate compilation unit. The
repository includes an example `plmnid` validator that is registered from
`examples/custom_validators.cpp` and validates a `PLMNID` string as a 3-digit
MCC plus a 2- or 3-digit MNC.

The validator registry is shared across parser instances. `clear()` removes
field bindings from a parser instance, but it does not erase globally
registered validator names.

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

`deserialise()` replaces previously parsed rows, columns, and
metadata, but preserve configured column types. Call `clear()` if you want to
reset both parsed data and configuration.

### Parse diagnostics

All diagnostics are available through `getDiagnostics()`. Errors and warnings
can also be queried separately:

```cpp
std::ifstream input("input.flg");
if (!input || !parser.deserialise(input)) {
    if (const ParseError* error = parser.getLastError()) {
        std::cerr << "Parse error at line " << error->line
                  << ", column " << error->column
                  << ": " << error->message << '\n';
    }
}

for (const auto& warning : parser.getWarnings()) {
    std::cerr << "Warning at line " << warning.line
              << ", column " << warning.column
              << ": " << warning.message << '\n';
}
```

Typical diagnostics include:

- unexpected type for a configured column
- missing required headings
- field validation failures
- too many delimiters in a key-value line
- too many delimiters in a data row
- too few fields in a data row
- invalid structure when the file does not match the expected `input.flg` shape

### Serialise back to FLG

```cpp
#include <fstream>

std::ofstream output("output.flg");
if (!output || !parser.serialise(output)) {
    std::cerr << "Serialisation failed\n";
}
```

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
- metadata numeric inference requires the full value to be numeric
- typed data columns are converted according to configured `ValueType`
- typed conversion also requires the full value to match the configured type
- non-empty invalid typed values produce parse errors with line and column
- booleans accept `true`/`false` case-insensitively and numeric `0`/non-zero
- empty cells are preserved as `std::string("")`

## Build And Run

```bash
bazel build //:example //:parser_examples
bazel test //:unit_tests
```

Enable the external `plmnid` validator for the example binaries with:

```bash
bazel build --define enable_custom_validators=true //:example //:parser_examples
```

Bazel requires both `flex` and `bison` to be installed because it generates the
lexer and parser sources during the build.

## Error Behavior

- open failures are reported by the caller when creating the `std::ifstream`
- `deserialise()` returns `false` for invalid structure, including marker-style
  input that does not match the `input.flg` shape
- warning diagnostics do not fail `deserialise()`
- error diagnostics, including missing required headings configured as errors,
  make `deserialise()` return `false`
- `serialise()` returns `false` if the output stream fails or the parser state
  cannot be emitted as a valid FLG document
- `getLastError()` returns the most recent parse diagnostic
- `getLastWarning()` returns the most recent warning diagnostic
- parse diagnostics are 1-based for both line and column numbers
- `getRow()` throws `std::out_of_range` for invalid row access
- `ValueVector::get<T>()` throws for bounds or type mismatches
- `ValueMap::get<T>()` throws `missing_key` or `wrong_type`
