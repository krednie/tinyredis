#include "value.h"
#include <new>

Value::Value() : type(ValueType::STRING), expiration(std::nullopt)
{
    new (&str) std::string();
}

Value::Value(const std::string& s) : type(ValueType::STRING)
{
    new (&str) std::string(s);
}

Value::Value(long long i) : type(ValueType::INTEGER), expiration(std::nullopt)
{
    integer = i;
}

Value::~Value()
{
    if (type == ValueType::STRING) {
        str.~basic_string();
    }
}

Value::Value(const Value& other) : type(other.type), expiration(other.expiration)
{
    if (type == ValueType::STRING) {
        new (&str) std::string(other.str);
    } else {
        integer = other.integer;
    }
}

Value& Value::operator=(const Value& other)
{
    if (this == &other)
        return *this;

    if (type == ValueType::STRING) {
        str.~basic_string();
    }

    type = other.type;
    expiration = other.expiration;

    if (type == ValueType::STRING) {
        new (&str) std::string(other.str);
    } else {
        integer = other.integer;
    }

    return *this;
}

bool Value::isExpired() const
{
    if(!expiration.has_value())
    {
        return false;
    }
    return std::chrono::steady_clock::now() >= expiration.value();
}

void Value::setExpiration(long long seconds)
{
    auto now = std::chrono::steady_clock::now();
    expiration = now + std::chrono::seconds(seconds);
}

void Value::persist()
{
    expiration = std::nullopt;
}
long long Value::getTTL() const
{
    if (!expiration.has_value()) {
        return -1;
    }
    
    auto now = std::chrono::steady_clock::now();
    if (now >= expiration.value()) {
        return -2;
    }
    
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(
        expiration.value() - now
    );
    
    return duration.count();
}