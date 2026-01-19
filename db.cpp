#include "db.h"
#include <iostream>

void Db::set(const std::string& key, const std::string& value)
{
    bucketstore[key] = Value(value);
    std::cout << "OK" << std::endl;
}

bool Db::get(const std::string& key)
{
    auto it = bucketstore.find(key);
    if (it == bucketstore.end()) {
        std::cout << "nil" << std::endl;
        return false;
    }

    Value &v = it->second;

    if (v.type == ValueType::STRING) {
        std::cout << v.str << std::endl;
        return true;
    }
    else if (v.type == ValueType::INTEGER)
    {
        std::cout << v.integer << std::endl;
        return true;
    }
    return false;

}

bool Db::del(const std::string& key)
{
    auto it = bucketstore.find(key);
    if (it == bucketstore.end()) {
        std::cout << "(integer) 0" << std::endl;
        return false;
    }

    bucketstore.erase(it);
    std::cout << "(integer) 1" << std::endl;
    return true;
}

bool Db::exists(const std::string& key)
{
    if (bucketstore.find(key) == bucketstore.end()) {
        std::cout << "(integer) 0" << std::endl;
        return false;
    }

    std::cout << "(integer) 1" << std::endl;
    return true;
}

bool Db::incr(const std::string &key)
{
    auto it = bucketstore.find(key);

    if (it == bucketstore.end()) 
    {
        bucketstore[key] = Value(1);
        std::cout << "(integer) 1" << std::endl;
        return true;
    }

    Value &v = it->second;

    if (v.type != ValueType::INTEGER) 
    {
        std::cout << "(error) WRONGTYPE" << std::endl;
        return false;
    }

    v.integer += 1;
    std::cout << "(integer) " << v.integer << std::endl;
    return true;
}

bool Db::incrby(const std::string& key, long long amount)
{
    auto it = bucketstore.find(key);

    if(it == bucketstore.end()) {
        bucketstore[key] = Value(amount);
        std::cout << "(integer) " << amount << std::endl;
        return true;
    }

    Value &v = it->second;

    if(v.type != ValueType::INTEGER)
    {
        std::cout << "(error) WRONGTYPE" << std::endl;
        return false;
    }

    v.integer += amount;
    std::cout << "(integer) " << v.integer << std::endl;
    return true;
}

bool Db::decr(const std::string& key)
{
    return incrby(key, -1);
}
bool Db::decrby(const std::string& key, long long amount)
{
    return incrby(key, -amount);
}

std::string Db::type(const std::string& key)
{
    auto it = bucketstore.find(key);
    if (it == bucketstore.end()) {
        std::cout << "none" << std::endl;
        return "none";
    }

    Value& v = it->second;

    if (v.type == ValueType::STRING) {
        std::cout << "string" << std::endl;
        return "string";
    }

    if (v.type == ValueType::INTEGER) {
        std::cout << "integer" << std::endl;
        return "integer";
    }

    return "none";
}