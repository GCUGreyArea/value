CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -MMD -MP -Ilib/include -Itest -I$(GENERATED_DIR)
LDFLAGS = -lgtest -lgtest_main -pthread

# Build directory
BUILD_DIR = build
GENERATED_DIR = $(BUILD_DIR)/generated

# Source files
LIB_SOURCES = lib/src/Parser.cpp lib/src/ParserConfig.cpp lib/src/ParserDriver.cpp $(GENERATED_DIR)/parser.tab.cpp $(GENERATED_DIR)/parser.yy.cpp
TEST_SOURCES = test/testValues.cpp test/testParser.cpp test/testParserConfig.cpp test/testValidatorRegistry.cpp
EXAMPLE_SOURCES = examples/example.cpp examples/parser_examples.cpp
VALIDATOR_SUPPORT_SOURCES = examples/custom_validators.cpp

# Object files in build directory
LIB_OBJECTS = $(addprefix $(BUILD_DIR)/,$(LIB_SOURCES:.cpp=.o))
TEST_OBJECTS = $(addprefix $(BUILD_DIR)/,$(TEST_SOURCES:.cpp=.o))
EXAMPLE_OBJECTS = $(addprefix $(BUILD_DIR)/,$(EXAMPLE_SOURCES:.cpp=.o))
VALIDATOR_SUPPORT_OBJECTS = $(addprefix $(BUILD_DIR)/,$(VALIDATOR_SUPPORT_SOURCES:.cpp=.o))

# Targets
all: unit_tests example parser_examples

unit_tests: $(LIB_OBJECTS) $(TEST_OBJECTS) $(VALIDATOR_SUPPORT_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

example: $(BUILD_DIR)/examples/example.o $(LIB_OBJECTS) $(VALIDATOR_SUPPORT_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^

parser_examples: $(BUILD_DIR)/examples/parser_examples.o $(LIB_OBJECTS) $(VALIDATOR_SUPPORT_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(GENERATED_DIR)/parser.tab.cpp $(GENERATED_DIR)/parser.tab.hpp: lib/src/parser.y
	@mkdir -p $(GENERATED_DIR)
	bison --defines=$(GENERATED_DIR)/parser.tab.hpp -o $(GENERATED_DIR)/parser.tab.cpp $<

$(GENERATED_DIR)/parser.yy.cpp: lib/src/parser.l $(GENERATED_DIR)/parser.tab.hpp
	@mkdir -p $(GENERATED_DIR)
	flex -o $@ $<

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(LIB_OBJECTS:.o=.d) $(TEST_OBJECTS:.o=.d) $(EXAMPLE_OBJECTS:.o=.d)

clean:
	rm -rf $(BUILD_DIR) unit_tests example parser_examples

.PHONY: all clean
