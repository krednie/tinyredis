#include "value.h"

Value::Value() : type(ValueType::STRING), data(std::string()), expiration(std::nullopt)
{
}

Value::Value(const std::string& s) : type(ValueType::STRING), data(s), expiration(std::nullopt)
{
}

Value::Value(long long i) : type(ValueType::INTEGER), data(i), expiration(std::nullopt)
{
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