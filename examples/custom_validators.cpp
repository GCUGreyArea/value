#include "Parser.hpp"
#include <cctype>

namespace {

bool isValidPlmnId(const std::string& value, std::string& errorMessage) {
    if (value.size() != 5 && value.size() != 6) {
        errorMessage =
            "PLMNID must be 5 or 6 digits: 3-digit MCC plus 2- or 3-digit MNC";
        return false;
    }

    for (char ch : value) {
        if (!std::isdigit(static_cast<unsigned char>(ch))) {
            errorMessage =
                "PLMNID must contain only digits: 3-digit MCC plus 2- or 3-digit MNC";
            return false;
        }
    }

    return true;
}

struct RegisterCustomValidators {
    RegisterCustomValidators() {
        registerValidator("plmnid", isValidPlmnId);
    }
};

RegisterCustomValidators registerCustomValidators;

} // namespace
