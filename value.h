#ifndef VALUE_H
#define VALUE_H
#include <chrono>
#include <optional>
#include <string>
#include <variant>

enum class ValueType
{
    STRING,
    INTEGER
};

struct Value
{
    ValueType type;
    std::variant<std::string, long long> data;  // Memory-efficient like union, but safe

    std::optional<std::chrono::time_point<std::chrono::steady_clock>> expiration;

    bool isExpired() const;
    void setExpiration(long long seconds);
    void persist();
    long long getTTL() const;

    // Accessors for convenience
    std::string& str() { return std::get<std::string>(data); }
    const std::string& str() const { return std::get<std::string>(data); }
    long long& integer() { return std::get<long long>(data); }
    long long integer() const { return std::get<long long>(data); }

    Value();
    Value(const std::string &s);
    Value(long long i);
    Value(const Value &other) = default;
    Value &operator=(const Value &other) = default;
    ~Value() = default;
};

#endif
