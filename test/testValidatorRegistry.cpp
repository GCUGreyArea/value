#include "Parser.hpp"
#include "ParserConfig.hpp"
#include <gtest/gtest.h>
#include <sstream>

class testValidatorRegistry : public ::testing::Test {
protected:
    FlgParser parser;
};

TEST_F(testValidatorRegistry, ValidateBuiltInAndExternalValidatorsAreRegistered) {
    EXPECT_TRUE(hasValidator("datetime_utc"));
    EXPECT_TRUE(hasValidator("plmnid"));
}

TEST_F(testValidatorRegistry, ValidateExternalPlmnIdValidatorCanBeBoundByName) {
    parser.setExpectedKeyValuePairs(1);
    parser.setFieldValidator("PLMNID", "plmnid");

    std::istringstream input(
        "META,value\n"
        "PLMNID\n"
        "23415\n");

    EXPECT_TRUE(parser.deserialise(input));
    EXPECT_TRUE(parser.getDiagnostics().empty());
}

TEST_F(testValidatorRegistry, ValidateExternalPlmnIdValidatorRejectsInvalidValues) {
    parser.setExpectedKeyValuePairs(1);
    parser.setFieldValidator("PLMNID", "plmnid");

    std::istringstream input(
        "META,value\n"
        "PLMNID\n"
        "23A15\n");

    EXPECT_FALSE(parser.deserialise(input));
    ASSERT_NE(parser.getLastError(), nullptr);
    EXPECT_NE(parser.getLastError()->message.find("PLMNID"), std::string::npos);
}

TEST_F(testValidatorRegistry, ValidateJsonConfigBindsExternalPlmnIdValidator) {
    const nlohmann::json config = {
        {"expected_key_value_pairs", 1},
        {"validators", {{"PLMNID", "plmnid"}}}};

    configureParserFromJson(config, parser);

    std::istringstream input(
        "META,value\n"
        "PLMNID\n"
        "001001\n");

    EXPECT_TRUE(parser.deserialise(input));
}
