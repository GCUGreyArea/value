#include "Values.hpp"
#include <gtest/gtest.h>

// Test fixture for ValueMap tests
class testValues : public ::testing::Test {
protected:
    ValueMap vm;
    ValueVector vv;
    
    void SetUp() override {
        vm.clear();
        vv.clear();
    }
};

// Validate ValueMap insert and size operations
TEST_F(testValues, ValidateValueMapInsertAndSize) {
    vm.set("int", 42);
    vm.set("uint", std::uint32_t(314));
    vm.set("string", std::string("hello"));
    vm.set("bool", true);
    EXPECT_EQ(vm.size(), 4);
}

// Validate ValueMap access and get operations
TEST_F(testValues, ValidateValueMapAccessAndGet) {
    vm.set("key", 100);
    EXPECT_EQ(vm.get<int>("key"), 100);
    EXPECT_THROW(vm.get<std::string>("key"), wrong_type);
    EXPECT_THROW(vm.get<int>("missing"), missing_key);
}

// Validate ValueMap contains operation
TEST_F(testValues, ValidateValueMapContains) {
    vm.set("exists", 42);
    EXPECT_TRUE(vm.contains("exists"));
    EXPECT_FALSE(vm.contains("nothere"));
}

// Validate ValueMap clear operation
TEST_F(testValues, ValidateValueMapClear) {
    vm.set("key1", 10);
    vm.set("key2", 20);
    EXPECT_EQ(vm.size(), 2);
    
    vm.clear();
    EXPECT_EQ(vm.size(), 0);
    EXPECT_FALSE(vm.contains("key1"));
}

// Validate ValueMap with multiple types
TEST_F(testValues, ValidateValueMapMultipleTypes) {
    vm.set("int_val", 42);
    vm.set("long_val", 123456789L);
    vm.set("uint8_val", std::uint8_t(255));
    vm.set("uint32_val", std::uint32_t(1000000));
    vm.set("string_val", std::string("test"));
    vm.set("bool_val", false);
    
    EXPECT_EQ(vm.size(), 6);
    EXPECT_EQ(vm.get<int>("int_val"), 42);
    EXPECT_EQ(vm.get<long>("long_val"), 123456789L);
    EXPECT_EQ(vm.get<std::uint8_t>("uint8_val"), 255u);
    EXPECT_EQ(vm.get<std::uint32_t>("uint32_val"), 1000000u);
    EXPECT_EQ(vm.get<std::string>("string_val"), "test");
    EXPECT_EQ(vm.get<bool>("bool_val"), false);
}

// Validate ValueMap type mismatch error
TEST_F(testValues, ValidateValueMapTypeMismatchError) {
    vm.set("value", 100);
    
    EXPECT_THROW(vm.get<std::string>("value"), wrong_type);
    EXPECT_THROW(vm.get<bool>("value"), wrong_type);
    EXPECT_THROW(vm.get<std::uint32_t>("value"), wrong_type);
}

// Validate ValueMap missing key error
TEST_F(testValues, ValidateValueMapMissingKeyError) {
    EXPECT_THROW(vm.get<int>("nonexistent"), missing_key);
}

// Validate ValueMap overwrite operation
TEST_F(testValues, ValidateValueMapOverwrite) {
    vm.set("key", 100);
    EXPECT_EQ(vm.get<int>("key"), 100);
    
    vm.set("key", 200);
    EXPECT_EQ(vm.get<int>("key"), 200);
    EXPECT_EQ(vm.size(), 1);
}

// Validate ValueVector push and size operations
TEST_F(testValues, ValidateValueVectorPushAndSize) {
    vv.push(1);
    vv.push(std::uint32_t(2));
    vv.push(std::string("world"));
    vv.push(false);
    EXPECT_EQ(vv.size(), 4);
}

// Validate ValueVector access and get operations
TEST_F(testValues, ValidateValueVectorAccessAndGet) {
    vv.push(10);
    vv.push(std::uint32_t(20));
    EXPECT_EQ(vv.get<int>(0), 10);
    EXPECT_EQ(vv.get<std::uint32_t>(1), 20u);
    EXPECT_THROW(vv.get<std::string>(2), std::out_of_range);
}

// Validate ValueVector bounds checking
TEST_F(testValues, ValidateValueVectorBoundsChecking) {
    vv.push(100);
    vv.push(200);
    
    EXPECT_EQ(vv.get<int>(0), 100);
    EXPECT_EQ(vv.get<int>(1), 200);
    EXPECT_THROW(vv.get<int>(2), std::out_of_range);
    EXPECT_THROW(vv.get<int>(100), std::out_of_range);
}

// Validate ValueVector raw access via at()
TEST_F(testValues, ValidateValueVectorAt) {
    vv.push(42);
    vv.push(std::string("hello"));

    EXPECT_EQ(std::get<int>(vv.at(0)), 42);
    EXPECT_EQ(std::get<std::string>(vv.at(1)), "hello");
    EXPECT_THROW(vv.at(2), std::out_of_range);
}

// Validate ValueVector type mismatch error
TEST_F(testValues, ValidateValueVectorTypeMismatchError) {
    vv.push(42);
    
    EXPECT_EQ(vv.get<int>(0), 42);
    EXPECT_THROW(vv.get<std::string>(0), std::runtime_error);
    EXPECT_THROW(vv.get<bool>(0), std::runtime_error);
}

// Validate ValueVector clear operation
TEST_F(testValues, ValidateValueVectorClear) {
    vv.push(1);
    vv.push(2);
    vv.push(3);
    EXPECT_EQ(vv.size(), 3);
    
    vv.clear();
    EXPECT_EQ(vv.size(), 0);
}

// Validate ValueVector with multiple types
TEST_F(testValues, ValidateValueVectorMultipleTypes) {
    vv.push(42);
    vv.push(123456789L);
    vv.push(std::uint8_t(255));
    vv.push(std::string("test"));
    vv.push(true);
    
    EXPECT_EQ(vv.size(), 5);
    EXPECT_EQ(vv.get<int>(0), 42);
    EXPECT_EQ(vv.get<long>(1), 123456789L);
    EXPECT_EQ(vv.get<std::uint8_t>(2), 255u);
    EXPECT_EQ(vv.get<std::string>(3), "test");
    EXPECT_EQ(vv.get<bool>(4), true);
}

// Validate ValueVector large collection
TEST_F(testValues, ValidateValueVectorLargeCollection) {
    const int SIZE = 1000;
    for (int i = 0; i < SIZE; ++i) {
        vv.push(i);
    }
    
    EXPECT_EQ(vv.size(), SIZE);
    EXPECT_EQ(vv.get<int>(0), 0);
    EXPECT_EQ(vv.get<int>(SIZE / 2), SIZE / 2);
    EXPECT_EQ(vv.get<int>(SIZE - 1), SIZE - 1);
}

// Validate ValueVector sequential access pattern
TEST_F(testValues, ValidateValueVectorSequentialAccess) {
    for (int i = 0; i < 10; ++i) {
        vv.push(i * 10);
    }
    
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(vv.get<int>(i), i * 10);
    }
}

// Validate ValueMap with various string types
TEST_F(testValues, ValidateValueMapStringHandling) {
    vm.set("empty", std::string(""));
    vm.set("short", std::string("hi"));
    vm.set("long", std::string("This is a longer string value for testing"));
    vm.set("special", std::string("Special!@#$%^&*()_+-=[]{}|;:,.<>?"));
    
    EXPECT_EQ(vm.get<std::string>("empty"), "");
    EXPECT_EQ(vm.get<std::string>("short"), "hi");
    EXPECT_GT(vm.get<std::string>("long").size(), 20);
    EXPECT_EQ(vm.get<std::string>("special"), "Special!@#$%^&*()_+-=[]{}|;:,.<>?");
}

// Validate ValueVector with uint64_t
TEST_F(testValues, ValidateValueVectorUint64) {
    std::uint64_t large_value = 18446744073709551615ull;
    vv.push(large_value);
    
    EXPECT_EQ(vv.get<std::uint64_t>(0), large_value);
}

// Validate ValueVector with negative long values
TEST_F(testValues, ValidateValueVectorNegativeLong) {
    vv.push(-9876543210L);
    
    EXPECT_EQ(vv.get<long>(0), -9876543210L);
}

// Validate ValueMap overwrite with different types
TEST_F(testValues, ValidateValueMapOverwriteWithDifferentType) {
    vm.set("key", 100);
    EXPECT_EQ(vm.get<int>("key"), 100);
    
    vm.set("key", std::string("now a string"));
    EXPECT_EQ(vm.get<std::string>("key"), "now a string");
}

// Validate empty ValueMap size
TEST_F(testValues, ValidateEmptyValueMapSize) {
    EXPECT_EQ(vm.size(), 0);
    EXPECT_FALSE(vm.contains("anything"));
}

// Validate empty ValueVector size
TEST_F(testValues, ValidateEmptyValueVectorSize) {
    EXPECT_EQ(vv.size(), 0);
}

// Validate ValueMap single entry
TEST_F(testValues, ValidateValueMapSingleEntry) {
    vm.set("only", 42);
    
    EXPECT_EQ(vm.size(), 1);
    EXPECT_TRUE(vm.contains("only"));
    EXPECT_EQ(vm.get<int>("only"), 42);
}

// Validate ValueVector single entry
TEST_F(testValues, ValidateValueVectorSingleEntry) {
    vv.push(999);
    
    EXPECT_EQ(vv.size(), 1);
    EXPECT_EQ(vv.get<int>(0), 999);
}

// Validate ValueMap uint16_t operations
TEST_F(testValues, ValidateValueMapUint16) {
    std::uint16_t value = 65535u;
    vm.set("max_uint16", value);
    
    EXPECT_EQ(vm.get<std::uint16_t>("max_uint16"), 65535u);
}

// Validate ValueVector uint32_t operations
TEST_F(testValues, ValidateValueVectorUint32) {
    std::uint32_t value = 4294967295u;
    vv.push(value);
    
    EXPECT_EQ(vv.get<std::uint32_t>(0), 4294967295u);
}
