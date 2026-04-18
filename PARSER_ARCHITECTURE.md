# FLG Parser Architecture and Design

## Overview

The FLG Parser is a flexible, configurable parser for `.flg` (File with
Key-Value pairs, Headers, and CSV data) files. The active runtime path in this
repository is the manual parser in `lib/src/Parser.cpp`, which integrates with
the `Value` type system for type-safe storage. The Flex/Bison sources remain in
the tree as reference grammar assets and possible future implementation paths.

## Architecture

### Components

```
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
    ┌────────────┼──────────────┐
    │            │              │
┌───▼─────┐ ┌────▼──────┐ ┌─────▼──┐
│ Manual  │ │ Reference │ │Value   │
│ Parser  │ │ Grammar   │ │System  │
│(.cpp)   │ │(l / y)    │ │        │
└─────────┘ └───────────┘ └────────┘
```

### Files

| File | Purpose |
|------|---------|
| `lib/include/Parser.hpp` | Parser API definition and configuration |
| `lib/include/Values.hpp` | Value type system (variants and containers) |
| `lib/src/Parser.cpp` | Active parser implementation and helpers |
| `lib/src/parser.l` | Reference lexer specification |
| `lib/src/parser.y` | Reference grammar specification |

## Design Decisions

### 1. Variant-Based Value Storage

**Decision**: Use `std::variant` to store heterogeneous types

**Rationale**:
- Type-safe at compile time
- No runtime overhead of virtual functions or dynamic casting
- Integrates well with C++'s type system
- Supports all necessary primitive types

**Trade-offs**:
- Must know type at compile time for access
- Error handling via exceptions for type mismatches

### 2. Reference Grammar Retention

**Decision**: Keep the Flex/Bison grammar files even though the active runtime
path is hand-written

**Rationale**:
- Preserves the intended grammar structure
- Keeps a migration path open for future parser generation work
- Documents tokenization and syntax separately from the runtime code

**Trade-offs**:
- Two parser descriptions must be kept aligned if the grammar files are revived
- The default build does not validate the Flex/Bison path

### 3. Two-Phase Type Configuration

**Decision**: Allow types to be set before or after header parsing

**Rationale**:
- By index: When column positions are known
- By name: When column names are known
- Can configure either before or after parsing

**Implementation**:
- `setColumnType(index, name, type)` - Configure by position
- `setColumnTypeByName(name, type)` - Configure by name
- Both store in `columnTypeMap` and `columnIndexTypeMap`
- Applied during data row parsing

### 4. String-Based Data Row Parsing

**Decision**: Parse data rows via string tokenization rather than pure Bison

**Rationale**:
- Simpler grammar handling
- Better error recovery
- Flexible whitespace handling
- Can be implemented with standard C++ functions

**Implementation**:
```cpp
// In lib/src/Parser.cpp parseString() method:
while (std::getline(ss, field, ',')) {
    Value parsed = parseValue(field, columns[col_index].type);
    row.push(parsed);
}
```

### 5. Configurable Column Types

**Decision**: Make column types fully configurable, not inferred from data

**Rationale**:
- Type safety: Prevents silent conversions
- Predictability: Parsing is deterministic
- Flexibility: Supports any schema configuration
- Performance: No scanning data twice

**Configuration Methods**:

**By Index (for CSV alignment)**:
```cpp
parser.setColumnType(0, "HEADING1", ValueType::FLOAT);
parser.setColumnType(1, "HEADING2", ValueType::STRING);
```

**By Name (for semantic clarity)**:
```cpp
parser.setColumnTypeByName("ID", ValueType::UINT32);
parser.setColumnTypeByName("ACTIVE", ValueType::BOOL);
```

## Data Flow

### Parsing Flow

```
Input File
    │
    ▼
┌────────────────┐
│ Manual Scanner │  → Line and field tokenization
└────────────────┘
    │
    ▼
┌────────────────┐
│ Header Detect  │  → Metadata/header boundary
└────────────────┘
    │
    ▼
┌────────────────┐
│ Type Convert   │  → Per-column conversion
└────────────────┘
    │
    ├─► KV Pairs → ValueMap
    ├─► Headers → ColumnDefinitions
    └─► Data Rows → ValueVectors
```

### Type Conversion Flow

```
String Input
    │
    ├─ Check configuration for column type
    │
    ▼
ParseValue(string, ValueType)
    │
    ├─ Apply type-specific conversion
    │   ├─ INT: std::stoi()
    │   ├─ FLOAT: std::stod()
    │   ├─ BOOL: isBoolean() + conversion
    │   └─ STRING: as-is
    │
    ▼
Value (variant)
    │
    ▼
Stored in ValueVector
```

## Extension Points

### 1. Adding New Value Types

To add a new type:

1. **Update enum** in `lib/include/Parser.hpp`:
```cpp
enum class ValueType {
    // ... existing types ...
    MY_NEW_TYPE
};
```

2. **Update Value variant** in `lib/include/Values.hpp`:
```cpp
using Value = std::variant<
    // ... existing types ...
    MyNewType
>;
```

3. **Update parseValue()** in `lib/src/Parser.cpp`:
```cpp
case ValueType::MY_NEW_TYPE:
    // Parsing logic
    break;
```

4. **Update valueToString()** in `lib/src/Parser.cpp`:
```cpp
else if (std::holds_alternative<MyNewType>(v)) {
    return // conversion to string
}
```

### 2. Changing Delimiters

To use a different delimiter:

1. **Update reference lexer** in `lib/src/parser.l`:
```flex
","    { return COMMA; }
```
Change to:
```flex
"|"    { return DELIMITER; }
```

2. **Update reference grammar** in `lib/src/parser.y`:
```bison
field_value { /* already works with any token */ }
```

3. **Update runtime parsing** in `lib/src/Parser.cpp`:
```cpp
while (std::getline(ss, field, ',')) {
```
Change to:
```cpp
while (std::getline(ss, field, '|')) {
```

### 3. Custom Value Validation

Implement post-parse validation:

```cpp
FlgParser parser;
// ... configure ...
parser.parseFile("input.flg");

// Validate
for (size_t i = 0; i < parser.getRowCount(); ++i) {
    const auto& row = parser.getRow(i);
    
    // Your validation logic
    if (someCondition(row)) {
        std::cerr << "Validation failed at row " << i << std::endl;
    }
}
```

### 4. Schema Configuration

Create helper functions for common configurations:

```cpp
class SchemaConfigurator {
public:
    static void configureStandardSchema(FlgParser& parser) {
        parser.setColumnType(0, "ID", ValueType::UINT32);
        parser.setColumnType(1, "NAME", ValueType::STRING);
        parser.setColumnType(2, "VALUE", ValueType::FLOAT);
        parser.setColumnType(3, "ACTIVE", ValueType::BOOL);
    }
    
    static void configureLegacySchema(FlgParser& parser) {
        parser.setColumnType(0, "KEY", ValueType::STRING);
        parser.setColumnType(1, "VAL", ValueType::STRING);
        // ... more columns ...
    }
};
```

## Performance Considerations

### Memory Usage

- **KV Pairs**: O(n) where n = number of key-value pairs
- **Column Definitions**: O(m) where m = number of columns
- **Data Rows**: O(n×m) where n = rows, m = columns

### Time Complexity

- **Parsing**: O(n×m) - linear in total tokens
- **Type Configuration**: O(1) per column
- **Data Access**: O(1) row access, O(1) column access within row

### Optimization Strategies

1. **Avoid repeated type lookups**:
   ```cpp
   // BAD: Looks up type multiple times
   for (size_t i = 0; i < 1000; ++i) {
       ValueType type = parser.getColumnType("NAME");
   }
   
   // GOOD: Look up once
   ValueType type = parser.getColumnType("NAME");
   for (size_t i = 0; i < 1000; ++i) {
       // Use type
   }
   ```

2. **Pre-allocate if row count is known**:
   ```cpp
   // Could implement reserve() method if needed
   parser.reserve(expectedRowCount);
   ```

3. **Use move semantics for large strings**:
   - Already handled by std::string

## Error Handling Strategy

### Parsing Errors
- Reported by Bison with line numbers
- Fatal - stops parsing
- Message format: "Parse error at line N: ..."

### Type Conversion Errors
- Caught during parseValue()
- Fall back to STRING type
- Non-fatal - parsing continues

### Access Errors
- Type mismatch: `wrong_type` exception
- Out of bounds: `std::out_of_range` exception
- Missing key: `missing_key` exception

## Testing Strategy

### Unit Tests (`test/testParser.cpp`)

1. **Basic parsing**: File format correctness
2. **Type configuration**: Index and name-based configuration
3. **Data access**: Row and column access patterns
4. **Type conversion**: Each ValueType
5. **Error conditions**: Bounds checking, type mismatches
6. **Edge cases**: Empty files, single row, whitespace

### Integration Tests

1. Parse real `.flg` files
2. Validate against schema
3. Compare with expected outputs

## Future Enhancements

1. **Streaming Parser**: For large files
2. **Schema Validation**: Enforce constraints
3. **Type Inference**: Auto-detect column types
4. **Custom Type Support**: User-defined types
5. **Compression Support**: Parse .flg.gz files
6. **Validation Functions**: Per-column validators
7. **Error Recovery**: Continue parsing on errors
8. **Statistics**: Row count, column statistics
9. **Export Formats**: JSON, XML output
10. **Performance Profiling**: Built-in timing

## Maintenance Notes

### When Modifying the Parser

1. **Grammar Changes**: Edit `lib/src/parser.y` if you intend to revive the
   generated parser path
2. **Lexer Changes**: Edit `lib/src/parser.l` alongside the grammar
3. **Runtime Parser Changes**: Keep `lib/src/Parser.cpp` aligned with the file
   format
4. **Test Changes**: Update `test/testParser.cpp`
5. **Documentation**: Update this file and `PARSER_USAGE.md`

### Build Requirements

- C++17 compiler
- GTest (for unit tests)

### Reference Grammar Notes

The default `Makefile` does not build the Flex/Bison grammar path. If that path
is reintroduced later, the generated artifacts and build rules will need to be
added intentionally rather than assumed to exist.
