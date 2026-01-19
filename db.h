#ifndef DB_H
#define DB_H

#include <unordered_map>
#include <string>
#include "value.h"

class Db
{
private:
    std::unordered_map<std::string, Value> bucketstore;

public:
    void set(const std::string &key, const std::string &value);
    bool get(const std::string &key);
    bool del(const std::string &key);
    bool exists(const std::string &key);
    bool incr(const std::string &key);
    bool incrby(const std::string &key, long long amount);
    bool decr(const std::string &key);
    bool decrby(const std::string &key, long long amount);
    std::string type(const std::string& key);

};

#endif
