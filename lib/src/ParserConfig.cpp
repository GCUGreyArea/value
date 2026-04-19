#include "ParserConfig.hpp"
#include <fstream>
#include <stdexcept>

namespace {

ValueType parseValueTypeName(const std::string& name) {
    if (name == "INT") {
        return ValueType::INT;
    }
    if (name == "LONG") {
        return ValueType::LONG;
    }
    if (name == "STRING") {
        return ValueType::STRING;
    }
    if (name == "UINT8") {
        return ValueType::UINT8;
    }
    if (name == "UINT16") {
        return ValueType::UINT16;
    }
    if (name == "UINT32") {
        return ValueType::UINT32;
    }
    if (name == "UINT64") {
        return ValueType::UINT64;
    }
    if (name == "BOOL") {
        return ValueType::BOOL;
    }
    if (name == "DOUBLE") {
        return ValueType::DOUBLE;
    }
    if (name == "FLOAT") {
        return ValueType::FLOAT;
    }

    throw std::runtime_error("Unknown ValueType: " + name);
}

DiagnosticSeverity parseHeadingRequirementSeverity(const std::string& requirement) {
    if (requirement == "error") {
        return DiagnosticSeverity::ERROR;
    }

    if (requirement == "warning") {
        return DiagnosticSeverity::WARNING;
    }

    throw std::runtime_error("Unknown field requirement: " + requirement);
}

} // namespace

nlohmann::json loadParserConfigFile(const std::string& configPath) {
    std::ifstream configStream(configPath);
    if (!configStream) {
        throw std::runtime_error("Cannot open config file: " + configPath);
    }

    nlohmann::json config;
    configStream >> config;
    return config;
}

void configureParserFromJson(const nlohmann::json& config, FlgParser& parser) {
    if (config.contains("expected_key_value_pairs")) {
        parser.setExpectedKeyValuePairs(
            config.at("expected_key_value_pairs").get<size_t>());
    }

    if (config.contains("delimiter")) {
        const std::string delimiter = config.at("delimiter").get<std::string>();
        if (delimiter.size() != 1) {
            throw std::runtime_error(
                "Config field 'delimiter' must contain exactly one character");
        }
        parser.setDelimiter(delimiter[0]);
    }

    if (config.contains("columns")) {
        const nlohmann::json& columns = config.at("columns");
        if (!columns.is_array()) {
            throw std::runtime_error("Config field 'columns' must be an array");
        }

        for (const auto& column : columns) {
            const size_t index = column.at("index").get<size_t>();
            const std::string name = column.at("name").get<std::string>();
            const std::string typeName = column.at("type").get<std::string>();
            parser.setColumnType(index, name, parseValueTypeName(typeName));
        }
    }

    if (config.contains("column_types_by_name")) {
        const nlohmann::json& columnTypesByName =
            config.at("column_types_by_name");
        if (!columnTypesByName.is_object()) {
            throw std::runtime_error(
                "Config field 'column_types_by_name' must be an object");
        }

        for (auto it = columnTypesByName.begin(); it != columnTypesByName.end();
             ++it) {
            parser.setColumnTypeByName(it.key(),
                                       parseValueTypeName(it.value().get<std::string>()));
        }
    }

    if (config.contains("field_requirements")) {
        const nlohmann::json& fieldRequirements = config.at("field_requirements");
        if (!fieldRequirements.is_object()) {
            throw std::runtime_error(
                "Config field 'field_requirements' must be an object");
        }

        for (auto it = fieldRequirements.begin(); it != fieldRequirements.end();
             ++it) {
            const std::string requirement = it.value().get<std::string>();
            if (requirement == "optional") {
                parser.setOptionalHeading(it.key());
                continue;
            }

            parser.setRequiredHeading(
                it.key(), parseHeadingRequirementSeverity(requirement));
        }
    }

    if (config.contains("key_value_requirements")) {
        const nlohmann::json& keyValueRequirements =
            config.at("key_value_requirements");
        if (!keyValueRequirements.is_object()) {
            throw std::runtime_error(
                "Config field 'key_value_requirements' must be an object");
        }

        for (auto it = keyValueRequirements.begin();
             it != keyValueRequirements.end();
             ++it) {
            const std::string requirement = it.value().get<std::string>();
            if (requirement == "optional") {
                parser.setOptionalKeyValuePair(it.key());
                continue;
            }

            parser.setRequiredKeyValuePair(
                it.key(), parseHeadingRequirementSeverity(requirement));
        }
    }

    if (config.contains("validators")) {
        const nlohmann::json& validators = config.at("validators");
        if (!validators.is_object()) {
            throw std::runtime_error(
                "Config field 'validators' must be an object");
        }

        for (auto it = validators.begin(); it != validators.end(); ++it) {
            parser.setFieldValidator(it.key(), it.value().get<std::string>());
        }
    }
}
