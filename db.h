#ifndef DB_H
#define DB_H

#include <unordered_map>
#include <string>
#include "value.h"

using namespace std;

class Db{
    private: 
    unordered_map<string, Value> bucketstore;

    public: 
    void set(const string &key, const string &value);
    bool get(const string &key);
    bool del(const string &key);
    bool exists (const string &key);
};

#endif