#include "db.h"
#include <iostream>

void Db::set(const std::string& key, const std::string& value) {
    bucketstore[key] = Value(value); 
    std::cout << "OK" << std::endl;
}

bool Db::get(const std::string& key) {
    auto it = bucketstore.find(key);
    if (it == bucketstore.end()) {
        std::cout << "nil" << std::endl;
        return false;
    }

    Value& v = it->second;

    if (v.type != ValueType::STRING) {
        std::cout << "(error) WRONGTYPE" << std::endl;
        return false;
    }

    std::cout << v.str << std::endl;
    return true;
}

bool Db::del(const std::string& key) {
    auto it = bucketstore.find(key);
    if (it == bucketstore.end()) {
        std::cout << "(integer) 0" << std::endl;
        return false;
    }

    bucketstore.erase(it);
    std::cout << "(integer) 1" << std::endl;
    return true;
}

bool Db::exists(const std::string& key) {
    if (bucketstore.find(key) == bucketstore.end()) {
        std::cout << "(integer) 0" << std::endl;
        return false;
    }

    std::cout << "(integer) 1" << std::endl;
    return true;
}
