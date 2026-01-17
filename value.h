#ifndef VALUE_H
#define VALUE_H

using namespace std;

#include <string>

enum class ValueType {
    STRING,
    INTEGER
};

struct Value {
    ValueType type;

    union {
        string str;
        long long integer;
    };
    Value();
    Value(const string s);
    Value(long long i);
    
    Value(const Value &other);
    Value &operator = (const Value &other);
    ~Value();
    
};
#endif