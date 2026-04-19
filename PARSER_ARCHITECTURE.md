# Flexible LAyout GRammer (FLG) Parser Architecture and Design

## Overview

The FLG parser is a configurable parser for `.flg` files containing:

- leading key-value metadata
- one header row
- zero or more data rows

The active runtime path uses a generated Flex lexer and Bison grammar, with a
parser driver layered on top to keep FLG-specific heuristics and diagnostics out
of the grammar itself.

## Architecture

### Components

```text
┌─────────────────────────────────────────┐
│     Application Code                    │
│  (tests, examples, user code)           │
└────────────────┬────────────────────────┘
                 │
        ┌────────▼────────┐
        │  FlgParser API  │
        │ (Parser.hpp)    │
        └────────┬────────┘
                 │
    ┌────────────┼──────────────┬─────────────┐
    │            │              │             │
┌───▼────────┐ ┌─▼───────────┐ ┌▼──────────┐ ┌▼───────┐
│ Parser     │ │ Flex Lexer  │ │ Bison     │ │ Value  │
│ Driver     │ │ (parser.l)  │ │ Grammar   │ │ System │
│            │ │             │ │ (parser.y)│ │        │
└────────────┘ └─────────────┘ └───────────┘ └────────┘
```

### Files

| File | Purpose |
|------|---------|
| `lib/include/Parser.hpp` | Parser API definition and configuration |
| `lib/include/ParserConfig.hpp` | Shared JSON configuration helpers |
| `lib/include/ParserDriver.hpp` | Driver-owned parser state types |
| `lib/include/Values.hpp` | Value type system (variants and containers) |
| `lib/src/Parser.cpp` | Public parser implementation and helpers |
| `lib/src/ParserConfig.cpp` | JSON config parsing and named validator binding |
| `lib/src/ParserDriver.cpp` | Driver implementation and diagnostics policy |
| `lib/src/parser.l` | Active lexer specification |
| `lib/src/parser.y` | Active grammar specification |
| `examples/custom_validators.cpp` | Example external validator registration unit |

## Design Decisions

### 1. Variant-Based Value Storage

**Decision**: Use `std::variant` to store heterogeneous types

**Rationale**:

- Type-safe at compile time
- No runtime overhead of virtual functions or dynamic casting
- Integrates well with C++'s type system
- Supports all necessary primitive types

### 2. Parser Driver Over Generated Grammar

**Decision**: Keep format-specific behavior in a parser driver layered over the
generated Flex/Bison parser

**Rationale**:

- Keeps grammar concerns focused on tokenization and row assembly
- Preserves configurable metadata/header heuristics outside the grammar
- Makes diagnostics, type conversion, and heading validation easier to test

**Trade-offs**:

- The driver and grammar must agree on row/token boundaries
- The build now depends on `flex` and `bison`

### 3. Two-Phase Type Configuration

**Decision**: Allow types to be set before or after header parsing

**Implementation**:

- `setColumnType(index, name, type)`
- `setColumnTypeByName(name, type)`
- Both store in `columnTypeMap` and `columnIndexTypeMap`
- The driver applies them when it materializes the header and data rows

### 4. Row-Oriented Grammar

**Decision**: Let Bison assemble rows while the driver decides whether each row
is metadata, header, or data

**Rationale**:

- Keeps the grammar compact despite FLG header ambiguity
- Preserves empty-field support
- Allows the driver to keep the current header-detection rules

**Implementation**:

```cpp
line:
    row NEWLINE { driver.consumeRow(*$1, @1.first_line, @2.first_column); }
    | row { driver.consumeRow(*$1, @1.first_line, @1.last_column + 1); }
```

### 5. Heading Requirements And Diagnostics

**Decision**: Missing required headings can be reported as either warnings or
errors

**Implementation**:

- `setRequiredHeading(name, severity)`
- `setOptionalHeading(name)`
- `setFieldValidator(field, validatorName)`
- `registerValidator(validatorName, validator)`
- `getDiagnostics()`, `getErrors()`, and `getWarnings()`

### 6. Field-Level String Validation

**Decision**: Keep string-format validation in the parser API and driver rather
than pushing it into the grammar.

**Rationale**:

- Validation rules are field-specific rather than syntax-level
- The same validator mechanism works for metadata keys and table columns
- Named validators can be bound from JSON config without changing the grammar

## Data Flow

### Parsing Flow

```text
Input Stream
    │
    ▼
┌────────────────┐
│ Flex Lexer     │  → Delimiters, newlines, field text
└────────────────┘
    │
    ▼
┌────────────────┐
│ Bison Grammar  │  → Row assembly
└────────────────┘
    │
    ▼
┌────────────────┐
│ Parser Driver  │  → Header detection, validation, conversion
└────────────────┘
    │
    ├─► KV Pairs → ValueMap
    ├─► Headers → ColumnDefinitions
    ├─► Data Rows → ValueVectors
    └─► Diagnostics → warnings / errors
```

## Extension Points

### 1. Adding New Value Types

To add a new type:

1. Update `ValueType` in `lib/include/Parser.hpp`.
2. Update `Value` in `lib/include/Values.hpp`.
3. Update `tryParseValue()` / `parseValue()` in `lib/src/Parser.cpp`.
4. Update `valueToString()` in `lib/src/Parser.cpp`.

### 2. Changing Delimiters

Delimiter behavior is runtime-configured through `setDelimiter()`. If lexer
tokenization rules need to change, update both `lib/src/parser.l` and the
driver logic in `lib/src/ParserDriver.cpp`.

### 3. Schema Configuration

Create helper functions for common configurations:

```cpp
class SchemaConfigurator {
public:
    static void configureStandardSchema(FlgParser& parser) {
        parser.setColumnType(0, "ID", ValueType::UINT32);
        parser.setColumnType(1, "NAME", ValueType::STRING);
        parser.setColumnType(2, "VALUE", ValueType::FLOAT);
        parser.setColumnType(3, "ACTIVE", ValueType::BOOL);
        parser.setRequiredHeading("ID", DiagnosticSeverity::ERROR);
    }
};
```

### 4. JSON Configuration

`lib/src/ParserConfig.cpp` maps JSON config into parser configuration:

- `field_requirements` -> `error`, `warning`, or `optional`
- `validators` -> field name to registered validator name

### 5. External Validator Registration

Validators can be registered from a separate compilation unit and then selected
from JSON or bound in code by name. The repository includes an external
`plmnid` validator example that validates a 3-digit MCC followed by a 2- or
3-digit MNC.

## Build Integration

`BUILD.bazel` runs `bison` and `flex` through Bazel `genrule`s, then compiles
the generated sources into the parser library and binaries.

The example binaries can optionally include `examples/custom_validators.cpp`
through a Bazel build flag:

```bash
bazel build --define enable_custom_validators=true //:example //:parser_examples
```
