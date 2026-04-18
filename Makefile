CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Ilib/include -Itest
LDFLAGS = -lgtest -lgtest_main -pthread

# Build directory
BUILD_DIR = build

# Source files
LIB_SOURCES = lib/src/Parser.cpp
TEST_SOURCES = test/testValues.cpp test/testParser.cpp
EXAMPLE_SOURCES = examples/example.cpp examples/parser_examples.cpp

# Object files in build directory
LIB_OBJECTS = $(addprefix $(BUILD_DIR)/,$(LIB_SOURCES:.cpp=.o))
TEST_OBJECTS = $(addprefix $(BUILD_DIR)/,$(TEST_SOURCES:.cpp=.o))
EXAMPLE_OBJECTS = $(addprefix $(BUILD_DIR)/,$(EXAMPLE_SOURCES:.cpp=.o))

# Targets
all: unit_tests example parser_examples

unit_tests: $(LIB_OBJECTS) $(TEST_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

example: $(BUILD_DIR)/examples/example.o $(LIB_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^

parser_examples: $(BUILD_DIR)/examples/parser_examples.o $(LIB_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) unit_tests example parser_examples

.PHONY: all clean
