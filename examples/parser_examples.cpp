#include "Parser.hpp"
#include <fstream>
#include <iostream>
#include <iomanip>

namespace {

bool loadInputFile(FlgParser& parser) {
    std::ifstream input("input.flg");
    return input && parser.deserialise(input);
}

} // namespace

/**
 * Example 1: Default parsing (all types as STRING)
 */
void example_default_parsing() {
    std::cout << "\n=== Example 1: Default Parsing ===" << std::endl;
    
    FlgParser parser;
    
    // No type configuration - all columns default to STRING
    if (loadInputFile(parser)) {
        std::cout << "Parsed successfully" << std::endl;
        std::cout << "Rows: " << parser.getRowCount() << std::endl;
        
        for (const auto& col : parser.getColumnDefinitions()) {
            std::cout << "  Column: " << col.name << std::endl;
        }
    }
}

/**
 * Example 2: Configure types by column index
 */
void example_configure_by_index() {
    std::cout << "\n=== Example 2: Configure by Index ===" << std::endl;
    
    FlgParser parser;
    
    // Configure types for known column positions
    parser.setColumnType(0, "HEADING1", ValueType::FLOAT);
    parser.setColumnType(1, "HEADING2", ValueType::STRING);
    parser.setColumnType(2, "HEADING3", ValueType::INT);
    parser.setColumnType(3, "HEADING4", ValueType::BOOL);
    parser.setColumnType(4, "HEADING5", ValueType::FLOAT);
    parser.setColumnType(5, "HEADING6", ValueType::UINT32);
    
    if (loadInputFile(parser)) {
        std::cout << "Parsed with configured types" << std::endl;
        
        if (parser.getRowCount() > 0) {
            const auto& row = parser.getRow(0);
            
            // Values are stored as Value variant, need type-safe access
            std::cout << "First row data:" << std::endl;
            for (size_t i = 0; i < row.size() && i < 6; ++i) {
                std::cout << "  Col " << i << ": "
                          << valueToString(row.at(i)) << std::endl;
            }
        }
    }
}

/**
 * Example 3: Configure types by column name
 */
void example_configure_by_name() {
    std::cout << "\n=== Example 3: Configure by Name ===" << std::endl;
    
    FlgParser parser;
    
    // Configure types by header name
    parser.setColumnTypeByName("HEADING1", ValueType::FLOAT);
    parser.setColumnTypeByName("HEADING4", ValueType::BOOL);
    
    if (loadInputFile(parser)) {
        std::cout << "Parsed with name-based configuration" << std::endl;
        std::cout << "Column HEADING1 type: " << static_cast<int>(parser.getColumnType("HEADING1")) << std::endl;
        std::cout << "Column HEADING4 type: " << static_cast<int>(parser.getColumnType("HEADING4")) << std::endl;
    }
}

/**
 * Example 4: Process data with type-safe access
 */
void example_type_safe_access() {
    std::cout << "\n=== Example 4: Type-Safe Access ===" << std::endl;
    
    FlgParser parser;
    
    parser.setColumnType(0, "HEADING1", ValueType::FLOAT);
    parser.setColumnType(2, "HEADING3", ValueType::INT);
    parser.setColumnType(3, "HEADING4", ValueType::BOOL);
    
    if (loadInputFile(parser)) {
        std::cout << std::left << std::setw(15) << "HEADING1" 
                  << std::setw(15) << "HEADING3"
                  << "HEADING4" << std::endl;
        std::cout << std::string(45, '-') << std::endl;
        
        for (size_t i = 0; i < parser.getRowCount(); ++i) {
            const auto& row = parser.getRow(i);
            
            try {
                float h1 = row.get<float>(0);
                int h3 = row.get<int>(2);
                bool h4 = row.get<bool>(3);
                
                std::cout << std::left << std::setw(15) << h1
                         << std::setw(15) << h3
                         << (h4 ? "true" : "false") << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Error processing row " << i << ": " << e.what() << std::endl;
            }
        }
    }
}

/**
 * Example 5: Access metadata (KV pairs)
 */
void example_access_metadata() {
    std::cout << "\n=== Example 5: Access Metadata ===" << std::endl;
    
    FlgParser parser;
    
    if (loadInputFile(parser)) {
        const auto& kv = parser.getKeyValuePairs();
        
        std::cout << "Key-Value Pairs in file:" << std::endl;
        std::cout << "Total KV pairs: " << kv.size() << std::endl;
    }
}

/**
 * Example 6: Filter and display specific columns
 */
void example_filter_columns() {
    std::cout << "\n=== Example 6: Filter Specific Columns ===" << std::endl;
    
    FlgParser parser;
    
    // Configure only the columns we care about
    parser.setColumnType(0, "HEADING1", ValueType::FLOAT);
    parser.setColumnType(3, "HEADING4", ValueType::BOOL);
    
    if (loadInputFile(parser)) {
        std::cout << "Displaying HEADING1 and HEADING4 only:" << std::endl;
        
        for (size_t i = 0; i < parser.getRowCount(); ++i) {
            const auto& row = parser.getRow(i);
            
            std::cout << "Row " << i << ": ";
            std::cout << valueToString(row.at(0)) << " | ";
            std::cout << valueToString(row.at(3)) << std::endl;
        }
    }
}

/**
 * Example 7: Validate and process with error handling
 */
void example_error_handling() {
    std::cout << "\n=== Example 7: Error Handling ===" << std::endl;
    
    FlgParser parser;
    
    parser.setColumnType(0, "HEADING1", ValueType::FLOAT);
    
    if (loadInputFile(parser)) {
        std::cout << "Processing rows with error handling:" << std::endl;
        
        for (size_t i = 0; i < parser.getRowCount(); ++i) {
            try {
                // Access with bounds checking
                parser.getRow(i);
                std::cout << "Row " << i << ": OK" << std::endl;
            } catch (const std::out_of_range& e) {
                std::cerr << "Row out of range: " << e.what() << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << std::endl;
            }
        }
    }
}

/**
 * Example 8: Dynamic type configuration based on file content
 */
void example_dynamic_configuration() {
    std::cout << "\n=== Example 8: Dynamic Configuration ===" << std::endl;
    
    FlgParser parser;
    
    // Configure based on application logic
    // For example, based on environment or config file
    bool use_strict_types = true;
    
    if (use_strict_types) {
        parser.setColumnType(0, "HEADING1", ValueType::FLOAT);
        parser.setColumnType(2, "HEADING3", ValueType::INT);
        parser.setColumnType(3, "HEADING4", ValueType::BOOL);
    }
    
    if (loadInputFile(parser)) {
        std::cout << "Parsed with dynamic configuration" << std::endl;
        std::cout << "Row count: " << parser.getRowCount() << std::endl;
    }
}

/**
 * Example 9: Multi-file parsing with configuration reuse
 */
void example_configuration_reuse() {
    std::cout << "\n=== Example 9: Configuration Reuse ===" << std::endl;
    
    // Define a standard configuration
    auto configure_standard = [](FlgParser& p) {
        p.setColumnType(0, "HEADING1", ValueType::FLOAT);
        p.setColumnType(1, "HEADING2", ValueType::STRING);
        p.setColumnType(2, "HEADING3", ValueType::INT);
        p.setColumnType(3, "HEADING4", ValueType::BOOL);
    };
    
    // Parse multiple files with same configuration
    FlgParser parser1;
    configure_standard(parser1);
    if (loadInputFile(parser1)) {
        std::cout << "File 1 parsed: " << parser1.getRowCount() << " rows" << std::endl;
    }
}

/**
 * Example 10: Display full file contents
 */
void example_display_full_file() {
    std::cout << "\n=== Example 10: Display Full File ===" << std::endl;
    
    FlgParser parser;
    
    // Configure all known columns
    for (size_t i = 0; i < 15; ++i) {
        parser.setColumnType(i, "HEADING" + std::to_string(i + 1), ValueType::STRING);
    }
    
    if (loadInputFile(parser)) {
        std::cout << "Columns: " << parser.getColumnDefinitions().size() << std::endl;
        std::cout << "Rows: " << parser.getRowCount() << std::endl;
        
        // Display header
        for (const auto& col : parser.getColumnDefinitions()) {
            std::cout << std::left << std::setw(20) << col.name;
        }
        std::cout << std::endl;
        
        // Display data
        for (size_t i = 0; i < parser.getRowCount(); ++i) {
            const auto& row = parser.getRow(i);
            for (size_t j = 0; j < row.size(); ++j) {
                std::cout << std::left << std::setw(20) 
                         << valueToString(row.at(j));
            }
            std::cout << std::endl;
        }
    }
}

int main() {
    std::cout << "FLG Parser Configuration Examples" << std::endl;
    std::cout << "=================================" << std::endl;
    
    try {
        example_default_parsing();
        example_configure_by_index();
        example_configure_by_name();
        example_type_safe_access();
        example_access_metadata();
        example_filter_columns();
        example_error_handling();
        example_dynamic_configuration();
        example_configuration_reuse();
        example_display_full_file();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "\n=== All Examples Complete ===" << std::endl;
    
    return 0;
}
