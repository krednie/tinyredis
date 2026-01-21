#ifndef VALUE_H
#define VALUE_H
#include <chrono>
#include <optional>
#include <string>

enum class ValueType
{
    STRING,
    INTEGER
};

struct Value
{

    ValueType type;
    union
    {
        std::string str;
        long long integer;
    };

    std::optional<std::chrono::time_point<std::chrono::steady_clock>> expiration;

    bool isExpired() const;
    void setExpiration(long long seconds);
    void persist();
    long long getTTL() const;

    Value();
    Value(const std::string &s);
    Value(long long i);
    Value(const Value &other);
    Value &operator=(const Value &other);

    ~Value();
};

#endif
