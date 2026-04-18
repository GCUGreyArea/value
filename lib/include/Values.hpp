#ifndef VALUES_HPP
#define VALUES_HPP

#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <variant>

using Value = std::variant<
    int,
    long,
    std::string,
    std::uint8_t,
    std::uint16_t,
    std::uint32_t,
    std::uint64_t,
    bool,
    double,
    float
>;

class missing_key : public std::runtime_error {
public:
    explicit missing_key(const std::string& key)
        : std::runtime_error("Missing key: " + key) {}
};

class wrong_type : public std::runtime_error {
public:
    explicit wrong_type(const std::string& key)
        : std::runtime_error("Wrong type for key: " + key) {}
};

class ValueMap {
private:
    std::unordered_map<std::string, Value> data;

public:
    template <typename T>
    void set(const std::string& key, const T& value) {
        data[key] = value;
    }

    template <typename T>
    const T& get(const std::string& key) const {
        auto it = data.find(key);
        if (it == data.end()) {
            throw missing_key(key);
        }

        if (const auto* p = std::get_if<T>(&it->second)) {
            return *p;
        }

        throw wrong_type(key);
    }

    size_t size() const {
        return data.size();
    }

    bool contains(const std::string& key) const {
        return data.find(key) != data.end();
    }

    void clear() {
        data.clear();
    }
};

class ValueVector {
private:
    std::vector<Value> data;

public:
    template <typename T>
    void push(const T& value) {
        data.push_back(value);
    }

    const Value& at(size_t index) const {
        if (index >= data.size()) {
            throw std::out_of_range("Index out of range: " +
                                    std::to_string(index));
        }

        return data[index];
    }

    template <typename T>
    const T& get(size_t index) const {
        const Value& value = at(index);

        if (const auto* p = std::get_if<T>(&value)) {
            return *p;
        }

        throw std::runtime_error("Wrong type at index: " + std::to_string(index));
    }

    size_t size() const {
        return data.size();
    }

    void clear() {
        data.clear();
    }
};

#endif // VALUES_HPP
