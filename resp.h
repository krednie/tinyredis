#ifndef RESP_H
#define RESP_H

#include <string>
#include <vector>

class RESP
{
public:
    // Parse RESP command from client (e.g., "*3\r\n$3\r\nSET\r\n...")
    static std::vector<std::string> parse(const std::string &data);
    
    // Encode responses to RESP format
    static std::string encodeSimpleString(const std::string &str);
    static std::string encodeError(const std::string &err);
    static std::string encodeInteger(long long num);
    static std::string encodeBulkString(const std::string &str);
    static std::string encodeNullBulkString();
    static std::string encodeArray(const std::vector<std::string> &items);
};

#endif