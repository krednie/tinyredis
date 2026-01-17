#include "value.h"
#include <new>

using namespace std;
Value::Value() : type(ValueType::STRING) 
{
    new (&str) string();
}

Value::Value(const string s) : type(ValueType::STRING) 
{
    new (&str) string(s);
}

Value::Value(long long i) : type(ValueType::INTEGER)
{
    integer = i;
}

Value::~Value() 
{
    if (type == ValueType::STRING)
    {
        str.~basic_string();
    }
}
Value::Value(const Value &other) :type(other.type) 
{
    if(type == ValueType::STRING)
    {
        new (&str) std::string(other.str);
    } 
    else
    {
        integer = other.integer;
    }
}
Value &Value::operator=(const Value &other)
{
    if (this == &other)
        return *this;

    if (type == ValueType::STRING)
    {
        str.~basic_string();
    }

    type = other.type;

    if (type == ValueType::STRING)
    {
        new (&str) std::string(other.str);
    }
    else
    {
        integer = other.integer;
    }

    return *this;
}