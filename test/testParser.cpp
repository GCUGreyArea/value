#include "Parser.hpp"
#include <gtest/gtest.h>
#include <sstream>
#include <fstream>

// Test fixture for parser tests
class testParser : public ::testing::Test {
protected:
    FlgParser parser;
    
    void SetUp() override {
        parser.clear();
    }
};

// Validate basic string parsing functionality
TEST_F(testParser, ValidateParseStringSimple) {
    std::string content = 
        "KEY1,100\n"
        "KEY2,text\n"
        "COL1,COL2,COL3\n"
        "10,hello,200\n"
        "20,world,300\n";
    
    EXPECT_TRUE(parser.parseString(content));
    EXPECT_EQ(parser.getRowCount(), 2);
    EXPECT_EQ(parser.getColumnDefinitions().size(), 3);
}

// Validate key-value pair extraction from file header
TEST_F(testParser, ValidateExtractKeyValuePairs) {
    std::string content =
        "KEY1,123\n"
        "KEY2,456\n"
        "COL1\n"
        "value1\n";

    parser.setExpectedKeyValuePairs(2);
    parser.parseString(content);
    const auto& kv = parser.getKeyValuePairs();
    
    EXPECT_TRUE(kv.contains("KEY1"));
    EXPECT_TRUE(kv.contains("KEY2"));
}

// Validate column definition parsing
TEST_F(testParser, ValidateColumnDefinitions) {
    std::string content =
        "KEY1,val\n"
        "HEADING1,HEADING2,HEADING3\n"
        "val1,val2,val3\n";
    
    parser.parseString(content);
    const auto& cols = parser.getColumnDefinitions();
    
    EXPECT_EQ(cols.size(), 3);
    EXPECT_EQ(cols[0].name, "HEADING1");
    EXPECT_EQ(cols[1].name, "HEADING2");
    EXPECT_EQ(cols[2].name, "HEADING3");
}

// Validate direct header row parsing without explicit marker
TEST_F(testParser, ValidateDirectHeaderRowParsing) {
    std::string content =
        "KEY1,123\n"
        "KEY2,text\n"
        "COL1,COL2,COL3\n"
        "10,hello,200\n"
        "20,world,300\n";

    EXPECT_TRUE(parser.parseString(content));
    EXPECT_EQ(parser.getColumnDefinitions().size(), 3);
    EXPECT_EQ(parser.getColumnDefinitions()[0].name, "COL1");
    EXPECT_EQ(parser.getColumnDefinitions()[2].name, "COL3");
    EXPECT_EQ(parser.getRowCount(), 2);
}

// Validate marker-style input is rejected
TEST_F(testParser, ValidateRejectsMarkerStyleInput) {
    std::string content =
        "KEY1,123\n"
        "HEADERS\n"
        "COL1,COL2,COL3\n"
        "10,hello,200\n";

    EXPECT_FALSE(parser.parseString(content));
}

// Validate parse error reports line and column for invalid type
TEST_F(testParser, ValidateTypeErrorReportsLocation) {
    std::string content =
        "KEY1,123\n"
        "ID,ACTIVE\n"
        "1,true\n"
        "2,maybe\n";

    parser.setColumnType(0, "ID", ValueType::INT);
    parser.setColumnType(1, "ACTIVE", ValueType::BOOL);
    parser.setExpectedKeyValuePairs(1);

    EXPECT_FALSE(parser.parseString(content));
    const ParseError* error = parser.getLastError();
    ASSERT_NE(error, nullptr);
    EXPECT_EQ(error->line, 4u);
    EXPECT_EQ(error->column, 3u);
    EXPECT_NE(error->message.find("Unexpected type"), std::string::npos);
}

// Validate parse error reports too many delimiters
TEST_F(testParser, ValidateTooManyDelimitersReportsLocation) {
    std::string content =
        "KEY1,123\n"
        "COL1,COL2\n"
        "10,20,30\n";

    parser.setExpectedKeyValuePairs(1);

    EXPECT_FALSE(parser.parseString(content));
    const ParseError* error = parser.getLastError();
    ASSERT_NE(error, nullptr);
    EXPECT_EQ(error->line, 3u);
    EXPECT_EQ(error->column, 7u);
    EXPECT_NE(error->message.find("Too many delimiters"), std::string::npos);
}

// Validate parser supports configurable delimiters
TEST_F(testParser, ValidateCustomDelimiter) {
    std::string content =
        "KEY1;123\n"
        "KEY2;hello\n"
        "COL1;COL2;COL3\n"
        "10;world;200\n";

    parser.setDelimiter(';');
    parser.setColumnType(0, "COL1", ValueType::INT);
    parser.setColumnType(1, "COL2", ValueType::STRING);
    parser.setColumnType(2, "COL3", ValueType::INT);

    EXPECT_TRUE(parser.parseString(content));
    ASSERT_EQ(parser.getRowCount(), 1);
    const auto& row = parser.getRow(0);
    EXPECT_EQ(row.get<int>(0), 10);
    EXPECT_EQ(row.get<std::string>(1), "world");
    EXPECT_EQ(row.get<int>(2), 200);
}

// Validate expected KV count enforces key-value structure
TEST_F(testParser, ValidateExpectedKeyValuePairErrorReportsLocation) {
    std::string content =
        "KEY1,123\n"
        "BROKEN,VALUE,EXTRA\n"
        "COL1,COL2\n"
        "10,20\n";

    parser.setExpectedKeyValuePairs(2);

    EXPECT_FALSE(parser.parseString(content));
    const ParseError* error = parser.getLastError();
    ASSERT_NE(error, nullptr);
    EXPECT_EQ(error->line, 2u);
    EXPECT_EQ(error->column, 13u);
    EXPECT_NE(error->message.find("Too many delimiters in key-value pair"),
              std::string::npos);
}

// Validate configuring column types by index
TEST_F(testParser, ValidateConfigureColumnTypeByIndex) {
    std::string content =
        "KEY,val\n"
        "COL1,COL2\n"
        "123,true\n";
    
    parser.setColumnType(0, "COL1", ValueType::INT);
    parser.setColumnType(1, "COL2", ValueType::BOOL);
    parser.setExpectedKeyValuePairs(1);
    
    parser.parseString(content);
    const auto& cols = parser.getColumnDefinitions();
    
    EXPECT_EQ(cols[0].type, ValueType::INT);
    EXPECT_EQ(cols[1].type, ValueType::BOOL);
}

// Validate configuring column types by header name
TEST_F(testParser, ValidateConfigureColumnTypeByName) {
    std::string content =
        "KEY,val\n"
        "VALUE,FLAG\n"
        "42,false\n";
    
    parser.setColumnTypeByName("VALUE", ValueType::INT);
    parser.setColumnTypeByName("FLAG", ValueType::BOOL);
    parser.setExpectedKeyValuePairs(1);
    
    parser.parseString(content);
    
    EXPECT_EQ(parser.getColumnType("VALUE"), ValueType::INT);
    EXPECT_EQ(parser.getColumnType("FLAG"), ValueType::BOOL);
}

// Validate accessing data rows
TEST_F(testParser, ValidateAccessDataRows) {
    std::string content =
        "KEY,val\n"
        "COL1,COL2,COL3\n"
        "10,hello,300\n"
        "20,world,400\n";
    
    parser.setColumnType(0, "COL1", ValueType::INT);
    parser.setColumnType(1, "COL2", ValueType::STRING);
    parser.setColumnType(2, "COL3", ValueType::INT);
    
    parser.parseString(content);
    
    EXPECT_EQ(parser.getRowCount(), 2);
    
    const auto& row0 = parser.getRow(0);
    EXPECT_EQ(row0.size(), 3);
    
    const auto& row1 = parser.getRow(1);
    EXPECT_EQ(row1.size(), 3);
}

// Validate out of bounds row access throws exception
TEST_F(testParser, ValidateOutOfBoundsRowAccess) {
    std::string content =
        "KEY,val\n"
        "COL1\n"
        "val1\n";

    parser.setExpectedKeyValuePairs(1);
    parser.parseString(content);
    
    EXPECT_THROW(parser.getRow(100), std::out_of_range);
}

// Validate parseValue function for INT type
TEST_F(testParser, ValidateParseValueTypeInt) {
    Value v_int = parseValue("42", ValueType::INT);
    EXPECT_EQ(std::get<int>(v_int), 42);
}

// Validate parseValue function for STRING type
TEST_F(testParser, ValidateParseValueTypeString) {
    Value v_str = parseValue("hello", ValueType::STRING);
    EXPECT_EQ(std::get<std::string>(v_str), "hello");
}

// Validate parseValue function for BOOL type (true)
TEST_F(testParser, ValidateParseValueTypeBoolTrue) {
    Value v_bool = parseValue("true", ValueType::BOOL);
    EXPECT_EQ(std::get<bool>(v_bool), true);
}

// Validate parseValue function for BOOL type (false)
TEST_F(testParser, ValidateParseValueTypeBoolFalse) {
    Value v_bool = parseValue("false", ValueType::BOOL);
    EXPECT_EQ(std::get<bool>(v_bool), false);
}

// Validate parseValue function for LONG type
TEST_F(testParser, ValidateParseValueTypeLong) {
    Value v_long = parseValue("9876543210", ValueType::LONG);
    EXPECT_EQ(std::get<long>(v_long), 9876543210L);
}

// Validate parseValue function for UINT8 type
TEST_F(testParser, ValidateParseValueTypeUint8) {
    Value v_uint8 = parseValue("255", ValueType::UINT8);
    EXPECT_EQ(std::get<std::uint8_t>(v_uint8), 255u);
}

// Validate parseValue function for UINT16 type
TEST_F(testParser, ValidateParseValueTypeUint16) {
    Value v_uint16 = parseValue("65535", ValueType::UINT16);
    EXPECT_EQ(std::get<std::uint16_t>(v_uint16), 65535u);
}

// Validate parseValue function for UINT32 type
TEST_F(testParser, ValidateParseValueTypeUint32) {
    Value v_uint32 = parseValue("4294967295", ValueType::UINT32);
    EXPECT_EQ(std::get<std::uint32_t>(v_uint32), 4294967295u);
}

// Validate parseValue function for UINT64 type
TEST_F(testParser, ValidateParseValueTypeUint64) {
    Value v_uint64 = parseValue("18446744073709551615", ValueType::UINT64);
    EXPECT_EQ(std::get<std::uint64_t>(v_uint64), 18446744073709551615ull);
}

// Validate valueToString conversion for INT
TEST_F(testParser, ValidateValueToStringInt) {
    Value v_int(42);
    EXPECT_EQ(valueToString(v_int), "42");
}

// Validate valueToString conversion for BOOL
TEST_F(testParser, ValidateValueToStringBool) {
    Value v_bool(true);
    EXPECT_EQ(valueToString(v_bool), "true");
}

// Validate valueToString conversion for STRING
TEST_F(testParser, ValidateValueToStringString) {
    Value v_str(std::string("hello"));
    EXPECT_EQ(valueToString(v_str), "hello");
}

// Validate multiple rows with consistent types
TEST_F(testParser, ValidateMultipleRowsConsistentTypes) {
    std::string content =
        "META,info\n"
        "ID,NAME,ACTIVE\n"
        "1,Alice,true\n"
        "2,Bob,false\n"
        "3,Charlie,true\n";
    
    parser.setColumnType(0, "ID", ValueType::INT);
    parser.setColumnType(1, "NAME", ValueType::STRING);
    parser.setColumnType(2, "ACTIVE", ValueType::BOOL);
    
    parser.parseString(content);
    
    EXPECT_EQ(parser.getRowCount(), 3);
    
    // Verify all rows have correct structure
    for (size_t i = 0; i < parser.getRowCount(); ++i) {
        const auto& row = parser.getRow(i);
        EXPECT_EQ(row.size(), 3);
    }
}

// Validate clear functionality resets parser state
TEST_F(testParser, ValidateClearFunctionality) {
    std::string content =
        "KEY,val\n"
        "COL1\n"
        "val1\n";

    parser.setExpectedKeyValuePairs(1);
    parser.parseString(content);
    EXPECT_EQ(parser.getRowCount(), 1);
    
    parser.clear();
    EXPECT_EQ(parser.getRowCount(), 0);
    EXPECT_EQ(parser.getColumnDefinitions().size(), 0);
}

// Validate whitespace handling in parsing
TEST_F(testParser, ValidateWhitespaceHandling) {
    std::string content =
        "KEY , val \n"
        "COL1 , COL2\n"
        " value1 , value2 \n";
    
    parser.setColumnType(0, "COL1", ValueType::STRING);
    parser.setColumnType(1, "COL2", ValueType::STRING);
    parser.setExpectedKeyValuePairs(1);
    
    parser.parseString(content);
    EXPECT_EQ(parser.getRowCount(), 1);
}

// Validate numeric string parsing for various types
TEST_F(testParser, ValidateNumericStringParsing) {
    std::string content =
        "KEY,val\n"
        "NUM1,NUM2,NUM3\n"
        "123,456789,999\n";
    
    parser.setColumnType(0, "NUM1", ValueType::INT);
    parser.setColumnType(1, "NUM2", ValueType::LONG);
    parser.setColumnType(2, "NUM3", ValueType::UINT32);
    
    parser.parseString(content);
    EXPECT_EQ(parser.getRowCount(), 1);
}

// Validate reparenting with different configurations
TEST_F(testParser, ValidateReparsingWithDifferentConfigurations) {
    std::string content =
        "KEY,val\n"
        "COL\n"
        "42\n";

    parser.setExpectedKeyValuePairs(1);
    // First parse with INT
    parser.setColumnType(0, "COL", ValueType::INT);
    parser.parseString(content);
    EXPECT_EQ(parser.getRowCount(), 1);
    
    // Clear and re-parse with STRING
    parser.clear();
    parser.setColumnType(0, "COL", ValueType::STRING);
    parser.parseString(content);
    EXPECT_EQ(parser.getRowCount(), 1);
}

// Validate reparsing replaces previous parsed data
TEST_F(testParser, ValidateReparsingResetsParsedData) {
    std::string first_content =
        "KEY,val\n"
        "COL1,COL2\n"
        "1,2\n"
        "3,4\n";

    std::string second_content =
        "META,other\n"
        "VALUE1,VALUE2,VALUE3\n"
        "5,6,7\n";

    parser.setExpectedKeyValuePairs(1);
    EXPECT_TRUE(parser.parseString(first_content));
    EXPECT_EQ(parser.getRowCount(), 2);
    EXPECT_EQ(parser.getColumnDefinitions().size(), 2);

    EXPECT_TRUE(parser.parseString(second_content));
    EXPECT_EQ(parser.getRowCount(), 1);
    EXPECT_EQ(parser.getColumnDefinitions().size(), 3);
    EXPECT_EQ(parser.getColumnDefinitions()[0].name, "VALUE1");
}

// Validate column type retrieval by name
TEST_F(testParser, ValidateGetColumnTypeByName) {
    std::string content =
        "KEY,val\n"
        "MYCOLUMN\n"
        "value\n";

    parser.setExpectedKeyValuePairs(1);
    parser.setColumnTypeByName("MYCOLUMN", ValueType::FLOAT);
    parser.parseString(content);
    
    EXPECT_EQ(parser.getColumnType("MYCOLUMN"), ValueType::FLOAT);
}

// Validate column type retrieval throws for missing column
TEST_F(testParser, ValidateGetColumnTypeThrowsForMissingColumn) {
    std::string content =
        "KEY,val\n"
        "COLUMN1\n"
        "value\n";

    parser.setExpectedKeyValuePairs(1);
    parser.parseString(content);
    
    EXPECT_THROW(parser.getColumnType("NONEXISTENT"), std::runtime_error);
}

// Validate parsing file with no data rows
TEST_F(testParser, ValidateParseFileWithNoDataRows) {
    std::string content =
        "KEY,val\n"
        "COL1,COL2\n";
    
    parser.setExpectedKeyValuePairs(1);
    parser.parseString(content);
    EXPECT_EQ(parser.getRowCount(), 0);
    EXPECT_EQ(parser.getColumnDefinitions().size(), 2);
}

// Validate parsing file with single column
TEST_F(testParser, ValidateParseFileWithSingleColumn) {
    std::string content =
        "KEY,val\n"
        "SINGLECOL\n"
        "value1\n"
        "value2\n";

    parser.setExpectedKeyValuePairs(1);
    parser.parseString(content);
    EXPECT_EQ(parser.getRowCount(), 2);
    EXPECT_EQ(parser.getColumnDefinitions().size(), 1);
}

// Validate boolean string case insensitivity
TEST_F(testParser, ValidateBooleanCaseInsensitivity) {
    Value v_true_lower = parseValue("true", ValueType::BOOL);
    Value v_true_upper = parseValue("TRUE", ValueType::BOOL);
    Value v_false_lower = parseValue("false", ValueType::BOOL);
    Value v_false_upper = parseValue("FALSE", ValueType::BOOL);
    
    EXPECT_EQ(std::get<bool>(v_true_lower), true);
    EXPECT_EQ(std::get<bool>(v_true_upper), true);
    EXPECT_EQ(std::get<bool>(v_false_lower), false);
    EXPECT_EQ(std::get<bool>(v_false_upper), false);
}

// Validate negative number parsing
TEST_F(testParser, ValidateNegativeNumberParsing) {
    Value v_neg_int = parseValue("-42", ValueType::INT);
    Value v_neg_long = parseValue("-9876543210", ValueType::LONG);
    
    EXPECT_EQ(std::get<int>(v_neg_int), -42);
    EXPECT_EQ(std::get<long>(v_neg_long), -9876543210L);
}

// Validate large field values
TEST_F(testParser, ValidateLargeFieldValues) {
    std::string content =
        "KEY,val\n"
        "LARGETEXT\n"
        "This is a very long string value that spans many characters to test large field handling\n";

    parser.setExpectedKeyValuePairs(1);
    parser.setColumnType(0, "LARGETEXT", ValueType::STRING);
    parser.parseString(content);
    
    EXPECT_EQ(parser.getRowCount(), 1);
    const auto& row = parser.getRow(0);
    std::string value = row.get<std::string>(0);
    EXPECT_GT(value.size(), 50);
}

// Validate empty middle field parsing
TEST_F(testParser, ValidateEmptyMiddleFieldParsing) {
    std::string content =
        "KEY,val\n"
        "COL1,COL2,COL3\n"
        "alpha,,omega\n";

    parser.parseString(content);

    ASSERT_EQ(parser.getRowCount(), 1);
    const auto& row = parser.getRow(0);
    EXPECT_EQ(row.size(), 3);
    EXPECT_EQ(row.get<std::string>(0), "alpha");
    EXPECT_EQ(row.get<std::string>(1), "");
    EXPECT_EQ(row.get<std::string>(2), "omega");
}

// Validate empty trailing field parsing
TEST_F(testParser, ValidateEmptyTrailingFieldParsing) {
    std::string content =
        "KEY,val\n"
        "COL1,COL2,COL3\n"
        "alpha,beta,\n";

    parser.parseString(content);

    ASSERT_EQ(parser.getRowCount(), 1);
    const auto& row = parser.getRow(0);
    EXPECT_EQ(row.size(), 3);
    EXPECT_EQ(row.get<std::string>(0), "alpha");
    EXPECT_EQ(row.get<std::string>(1), "beta");
    EXPECT_EQ(row.get<std::string>(2), "");
}

// Validate empty typed field falls back to empty string
TEST_F(testParser, ValidateEmptyTypedFieldFallback) {
    std::string content =
        "KEY,val\n"
        "COUNT,NAME\n"
        ",Alice\n";

    parser.setColumnType(0, "COUNT", ValueType::INT);
    parser.setColumnType(1, "NAME", ValueType::STRING);
    parser.setExpectedKeyValuePairs(1);

    parser.parseString(content);

    ASSERT_EQ(parser.getRowCount(), 1);
    const auto& row = parser.getRow(0);
    EXPECT_EQ(row.get<std::string>(0), "");
    EXPECT_EQ(row.get<std::string>(1), "Alice");
}

// Validate many columns
TEST_F(testParser, ValidateManyColumns) {
    std::string content =
        "KEY,val\n"
        "C1,C2,C3,C4,C5,C6,C7,C8,C9,C10\n"
        "1,2,3,4,5,6,7,8,9,10\n";
    
    for (size_t i = 0; i < 10; ++i) {
        parser.setColumnType(i, "C" + std::to_string(i + 1), ValueType::INT);
    }
    
    parser.parseString(content);
    
    EXPECT_EQ(parser.getColumnDefinitions().size(), 10);
    EXPECT_EQ(parser.getRowCount(), 1);
    const auto& row = parser.getRow(0);
    EXPECT_EQ(row.size(), 10);
}

// Validate mixed type configuration
TEST_F(testParser, ValidateMixedTypeConfiguration) {
    std::string content =
        "KEY,val\n"
        "ID,NAME,SCORE,ACTIVE,TIMESTAMP\n"
        "1,Alice,95.5,true,1234567890\n";
    
    parser.setColumnType(0, "ID", ValueType::UINT32);
    parser.setColumnType(1, "NAME", ValueType::STRING);
    parser.setColumnType(2, "SCORE", ValueType::FLOAT);
    parser.setColumnType(3, "ACTIVE", ValueType::BOOL);
    parser.setColumnType(4, "TIMESTAMP", ValueType::LONG);
    
    parser.parseString(content);
    
    EXPECT_EQ(parser.getRowCount(), 1);
    const auto& row = parser.getRow(0);
    EXPECT_EQ(row.size(), 5);
}

// Validate parsing of input.flg file from resources
TEST_F(testParser, ValidateParseInputFile) {
    std::ifstream file("test/resources/input.flg");
    ASSERT_TRUE(file.is_open()) << "Failed to open test/resources/input.flg";
    
    // Read file content
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    
    // Parse the file
    EXPECT_TRUE(parser.parseString(buffer.str()));
    
    // Validate structure
    const auto& kv = parser.getKeyValuePairs();
    EXPECT_TRUE(kv.contains("KEYONE"));
    EXPECT_TRUE(kv.contains("KEYTWO"));
    EXPECT_TRUE(kv.contains("KEYTHREE"));
    EXPECT_TRUE(kv.contains("KEYFOUR"));
    EXPECT_TRUE(kv.contains("KEYFIVE"));
    
    // Validate columns
    const auto& cols = parser.getColumnDefinitions();
    EXPECT_EQ(cols.size(), 15);
    EXPECT_EQ(cols[0].name, "HEADING1");
    EXPECT_EQ(cols[14].name, "HEADING15");
    
    // Validate data rows
    EXPECT_EQ(parser.getRowCount(), 2);
    
    const auto& row1 = parser.getRow(0);
    EXPECT_EQ(row1.size(), 15);
    
    const auto& row2 = parser.getRow(1);
    EXPECT_EQ(row2.size(), 15);
}
