#include "Parser.hpp"
#include "ParserConfig.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

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
        const nlohmann::json config = loadParserConfigFile(config_path);

        FlgParser parser;
        configureParserFromJson(config, parser);

        std::ifstream input_stream(input_path);
        if (!input_stream) {
            std::cerr << "Cannot open input file: " << input_path << std::endl;
            return 1;
        }

        if (!parser.deserialise(input_stream)) {
            if (const ParseError* error = parser.getLastError()) {
                std::cerr << "Parse error at line " << error->line
                          << ", column " << error->column << ": "
                          << error->message << std::endl;
            } else {
                std::cerr << "Deserialisation failed with no diagnostic" << std::endl;
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
