# FLG Parser Implementation

## Current State

The repository currently uses a manual line-oriented parser implemented in
`lib/src/Parser.cpp`.

The files below also exist:

- `lib/src/parser.l`
- `lib/src/parser.y`

They describe a Flex/Bison grammar, but they are not compiled or linked by the
default build. They should be treated as reference material until that path is
intentionally restored.

## Runtime Components

### `lib/include/Values.hpp`

Defines:

- `Value`, the `std::variant` backing type
- `ValueMap`, named heterogeneous values
- `ValueVector`, ordered heterogeneous values

### `lib/include/Parser.hpp`

Defines:

- `ValueType`
- `ColumnDefinition`
- `ParseError`
- `FlgParser`
- `parseValue()`
- `valueToString()`

### `lib/src/Parser.cpp`

Implements:

- parser configuration methods
- delimiter configuration
- file and string parsing
- column header detection
- row conversion into typed `ValueVector` instances
- parse diagnostics with line/column tracking
- string conversion helpers

## Parser Flow

`parseString()` works in three phases:

1. Clear previously parsed metadata, columns, and rows while preserving column
   type configuration.
2. Read metadata rows until it detects the header boundary.
3. Parse each remaining row according to configured or default column types.

Header detection supports:

- direct header rows with multiple comma-separated columns
- fixed metadata counts via `setExpectedKeyValuePairs()`

The parser delimiter is configurable via `setDelimiter()`.

## Type Handling

- metadata values are inferred as `double`, `long`, or `std::string`
- data row fields are converted via `parseValue()`
- non-empty invalid typed values are reported as parse errors
- empty cells are preserved as `std::string("")`
- `valueToString()` provides display conversion across every `Value` variant
  alternative

## Build

The active build is the top-level `Makefile`.

```bash
make
```

Targets:

- `unit_tests`
- `example`
- `parser_examples`

## Tests

The test suite lives in:

- `test/testValues.cpp`
- `test/testParser.cpp`

Coverage includes:

- value container behavior
- parser configuration
- delimiter configuration
- parse error line and column reporting
- direct header rows
- repeated parsing without stale-state accumulation
- parsing the repository sample file in `test/resources/input.flg`

Run with:

```bash
./unit_tests
```
