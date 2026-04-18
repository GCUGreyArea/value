#include "Parser.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

namespace {

using json = nlohmann::json;

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

json loadConfigFile(const std::string& config_path) {
    std::ifstream config_stream(config_path);
    if (!config_stream) {
        throw std::runtime_error("Cannot open config file: " + config_path);
    }

    json config;
    config_stream >> config;
    return config;
}

void configureParser(const json& config, FlgParser& parser) {
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
        const json& columns = config.at("columns");
        if (!columns.is_array()) {
            throw std::runtime_error("Config field 'columns' must be an array");
        }

        for (const auto& column : columns) {
            const size_t index = column.at("index").get<size_t>();
            const std::string name = column.at("name").get<std::string>();
            const std::string type_name = column.at("type").get<std::string>();
            parser.setColumnType(index, name, parseValueTypeName(type_name));
        }
    }

    if (config.contains("column_types_by_name")) {
        const json& column_types_by_name = config.at("column_types_by_name");
        if (!column_types_by_name.is_object()) {
            throw std::runtime_error(
                "Config field 'column_types_by_name' must be an object");
        }

        for (auto it = column_types_by_name.begin();
             it != column_types_by_name.end(); ++it) {
            parser.setColumnTypeByName(it.key(),
                                       parseValueTypeName(it.value().get<std::string>()));
        }
    }
}

void printParseSummary(const FlgParser& parser) {
    std::cout << "Parsed successfully" << std::endl;
    std::cout << "Rows: " << parser.getRowCount() << std::endl;
    std::cout << "Columns: " << parser.getColumnDefinitions().size()
              << std::endl;
    std::cout << "KV pairs: " << parser.getKeyValuePairs().size() << std::endl;

    if (!parser.getColumnDefinitions().empty()) {
        std::cout << "\nColumn Definitions:" << std::endl;
        for (const auto& col : parser.getColumnDefinitions()) {
            std::cout << "  - " << col.name << std::endl;
        }
    }

    if (parser.getRowCount() > 0) {
        const auto& first_row = parser.getRow(0);
        std::cout << "\nFirst Row:" << std::endl;
        for (size_t i = 0; i < first_row.size(); ++i) {
            std::cout << "  [" << i << "] " << valueToString(first_row.at(i))
                      << std::endl;
        }
    }
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0]
                  << " <config.json> <input-file>" << std::endl;
        return 1;
    }

    const std::string config_path = argv[1];
    const std::string input_path = argv[2];

    try {
        const json config = loadConfigFile(config_path);

        FlgParser parser;
        configureParser(config, parser);

        if (!parser.parseFile(input_path)) {
            if (const ParseError* error = parser.getLastError()) {
                std::cerr << "Parse error at line " << error->line
                          << ", column " << error->column << ": "
                          << error->message << std::endl;
            } else {
                std::cerr << "Parse failed with no diagnostic" << std::endl;
            }
            return 1;
        }

        printParseSummary(parser);
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Configuration error: " << e.what() << std::endl;
        return 1;
    }
}
