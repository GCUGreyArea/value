#include "ParserConfig.hpp"
#include <gtest/gtest.h>
#include <sstream>

class testParserConfig : public ::testing::Test {
protected:
    FlgParser parser;
};

TEST_F(testParserConfig, ValidateConfigSetsFieldRequirementsAndValidators) {
    const nlohmann::json config = {
        {"expected_key_value_pairs", 1},
        {"field_requirements",
         {{"ID", "error"}, {"COMMENT", "warning"}, {"EXTRA", "optional"}}},
        {"validators", {{"STAMP", "datetime_utc"}}}};

    configureParserFromJson(config, parser);

    std::istringstream input(
        "STAMP,2026-04-19T12:34:56Z\n"
        "VALUE\n"
        "42\n");

    EXPECT_FALSE(parser.deserialise(input));
    EXPECT_EQ(parser.getWarnings().size(), 1u);
    ASSERT_NE(parser.getLastError(), nullptr);
    EXPECT_NE(parser.getLastError()->message.find("Required heading missing: ID"),
              std::string::npos);
}

TEST_F(testParserConfig, ValidateConfigSupportsOptionalFields) {
    const nlohmann::json config = {
        {"expected_key_value_pairs", 1},
        {"field_requirements", {{"COMMENT", "optional"}}}};

    configureParserFromJson(config, parser);

    std::istringstream input(
        "META,value\n"
        "VALUE\n"
        "42\n");

    EXPECT_TRUE(parser.deserialise(input));
    EXPECT_TRUE(parser.getDiagnostics().empty());
}

TEST_F(testParserConfig, ValidateConfigSetsKeyValueRequirements) {
    const nlohmann::json config = {
        {"expected_key_value_pairs", 1},
        {"key_value_requirements",
         {{"MISSING_ERROR", "error"},
          {"MISSING_WARNING", "warning"},
          {"IGNORED", "optional"}}}};

    configureParserFromJson(config, parser);

    std::istringstream input(
        "META,value\n"
        "VALUE\n"
        "42\n");

    EXPECT_FALSE(parser.deserialise(input));
    EXPECT_EQ(parser.getWarnings().size(), 1u);
    ASSERT_NE(parser.getLastError(), nullptr);
    EXPECT_NE(
        parser.getLastError()->message.find("Required key-value pair missing"),
        std::string::npos);
}

TEST_F(testParserConfig, ValidateConfigRejectsUnknownValidatorName) {
    const nlohmann::json config = {{"validators", {{"STAMP", "missing"}}}};

    EXPECT_THROW(configureParserFromJson(config, parser), std::runtime_error);
}
