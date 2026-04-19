#ifndef PARSER_CONFIG_HPP
#define PARSER_CONFIG_HPP

#include "Parser.hpp"
#include <nlohmann/json.hpp>
#include <string>

nlohmann::json loadParserConfigFile(const std::string& configPath);
void configureParserFromJson(const nlohmann::json& config, FlgParser& parser);

#endif // PARSER_CONFIG_HPP
