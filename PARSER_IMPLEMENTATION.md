# Flexible Layout Grammer (FLG) Parser Implementation

## Current State

The repository now uses a generated parser built from Flex and Bison. The
driver in `lib/src/ParserDriver.cpp` owns FLG-specific state and policy,
while `lib/src/Parser.cpp` exposes the public API and serialization helpers.

## Runtime Components

### `lib/include/Values.hpp`

Defines:

- `Value`, the `std::variant` backing type
- `ValueMap`, named heterogeneous values
- `ValueVector`, ordered heterogeneous values

### `lib/include/Parser.hpp`

Defines:

- `ValueType`
- `DiagnosticSeverity`
- `ColumnDefinition`
- `ParseDiagnostic` / `ParseError`
- `StringFieldValidator`
- `FlgParser`
- validator registry helpers such as `registerValidator()`
- `tryParseValue()`
- `parseValue()`
- `valueToString()`

### `lib/include/ParserConfig.hpp`

Defines:

- JSON-backed parser configuration helpers for examples and tests

### `lib/include/ParserDriver.hpp`

Defines:

- `ParsedField`
- `ParsedRow`
- `ParserDriver`

### `lib/src/Parser.cpp`

Implements:

- parser configuration methods
- required-heading configuration
- field-to-validator binding
- global validator registry access
- stream serialisation
- driver-backed `deserialise()`
- diagnostic accessors
- value conversion helpers

### `lib/src/ParserDriver.cpp`

Implements:

- parser-state reset while preserving configured schema
- metadata/header boundary heuristics
- required-heading validation
- field-level validator execution for metadata and tabular fields through the
  named registry
- warning/error diagnostic collection
- typed row conversion into `ValueVector` instances
- stream input handoff to the generated parser

### `lib/src/parser.l`

Implements:

- delimiter and newline tokenization
- empty-field synthesis
- driver-backed scanner input
- token location tracking

### `lib/src/parser.y`

Implements:

- row assembly from lexer tokens
- dispatch of parsed rows into the driver

## Parser Flow

`deserialise()` works in three phases:

1. Reset previously parsed metadata, columns, rows, and diagnostics while
   preserving parser configuration.
2. Run the Flex/Bison parser to assemble rows and hand them to `ParserDriver`.
3. Let the driver classify rows as metadata, header, or data, then perform type
   conversion and required-heading checks.

Header detection supports:

- direct header rows with multiple delimited columns
- fixed metadata counts via `setExpectedKeyValuePairs()`
- required headings reported as warnings or errors

The parser delimiter is configurable via `setDelimiter()`.

## Type Handling

- metadata values are inferred as `double`, `long`, or `std::string`
- metadata numeric inference only succeeds when the full field is numeric
- data row fields are converted via `tryParseValue()` / `parseValue()`
- field validators run against trimmed string values after field identification
  and are resolved from the named registry
- non-empty invalid typed values are reported as error diagnostics
- empty cells are preserved as `std::string("")`
- `valueToString()` provides display conversion across every `Value` variant
  alternative

## Build

The primary build is Bazel.

```bash
bazel build //:example //:parser_examples
bazel test //:unit_tests
```

Targets:

- `unit_tests`
- `example`
- `parser_examples`

Enable the external `plmnid` validator in the example binaries with:

```bash
bazel build --define enable_custom_validators=true //:example //:parser_examples
```

The generated parser path requires both `flex` and `bison`.

## Tests

The test suite lives in:

- `test/testValues.cpp`
- `test/testParser.cpp`
- `test/testParserConfig.cpp`
- `test/testValidatorRegistry.cpp`

Coverage includes:

- value container behavior
- parser configuration
- JSON config loading
- delimiter configuration
- stream serialisation and deserialisation
- parse diagnostic line and column reporting
- required-heading warnings and errors
- field validators, including UTC datetime validation
- external validator registration from a separate compilation unit
- large-input parsing
- direct header rows
- repeated parsing without stale-state accumulation
- parsing the repository sample file in `test/resources/input.flg`

Run with:

```bash
./unit_tests
```
