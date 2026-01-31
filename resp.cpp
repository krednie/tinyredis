#include "resp.h"
#include <iostream>
#include <sstream>

// Helper function to parse plain text commands (like "SET key value")
static std::vector<std::string> parsePlainText(const std::string &data)
{
    std::vector<std::string> result;
    
    // Trim trailing \r\n or \n
    std::string trimmed = data;
    while (!trimmed.empty() && (trimmed.back() == '\r' || trimmed.back() == '\n'))
    {
        trimmed.pop_back();
    }
    
    std::istringstream iss(trimmed);
    std::string word;
    while (iss >> word)
    {
        result.push_back(word);
    }
    
    return result;
}

std::vector<std::string> RESP::parse(const std::string &data)
{
    std::vector<std::string> result;
    size_t pos = 0;

    std::cout << "# [RESP Parser] Parsing data of length " << data.length() << std::endl;

    // Check if this is RESP format (starts with '*') or plain text
    if (data.empty())
    {
        return result;
    }
    
    if (data[0] != '*')
    {
        // Not RESP format - parse as plain text command
        std::cout << "# [RESP Parser] Plain text mode - parsing as natural language" << std::endl;
        return parsePlainText(data);
    }

    // RESP parsing (original logic)
    // Skip '*' and read array length
    pos = 1;
    size_t end = data.find("\r\n", pos);
    if (end == std::string::npos)
    {
        std::cout << "# [RESP Parser] No \\r\\n found after *" << std::endl;
        return result;
    }

    int arrayLen = std::stoi(data.substr(pos, end - pos));
    std::cout << "# [RESP Parser] Array length: " << arrayLen << std::endl;
    pos = end + 2;

    // Parse each bulk string
    for (int i = 0; i < arrayLen; i++)
    {
        std::cout << "# [RESP Parser] Parsing element " << (i + 1) << "/" << arrayLen << std::endl;

        if (pos >= data.length() || data[pos] != '$')
        {
            std::cout << "# [RESP Parser] Expected $ at position " << pos << std::endl;
            break;
        }

        pos++; // Skip '$'
        end = data.find("\r\n", pos);
        if (end == std::string::npos)
        {
            std::cout << "# [RESP Parser] No \\r\\n found after $" << std::endl;
            break;
        }

        int bulkLen = std::stoi(data.substr(pos, end - pos));
        std::cout << "# [RESP Parser] Bulk string length: " << bulkLen << std::endl;
        pos = end + 2;

        if (pos + bulkLen > data.length())
        {
            std::cout << "# [RESP Parser] Not enough data for bulk string" << std::endl;
            break;
        }

        std::string bulk = data.substr(pos, bulkLen);
        std::cout << "# [RESP Parser] Extracted: \"" << bulk << "\"" << std::endl;
        result.push_back(bulk);
        pos += bulkLen + 2; // Skip bulk string and \r\n
    }

    std::cout << "# [RESP Parser] Parsed " << result.size() << " tokens" << std::endl;
    return result;
}

std::string RESP::encodeSimpleString(const std::string &str)
{
    return "+" + str + "\r\n";
}

std::string RESP::encodeError(const std::string &err)
{
    return "-" + err + "\r\n";
}

std::string RESP::encodeInteger(long long num)
{
    return ":" + std::to_string(num) + "\r\n";
}

std::string RESP::encodeBulkString(const std::string &str)
{
    return "$" + std::to_string(str.length()) + "\r\n" + str + "\r\n";
}

std::string RESP::encodeNullBulkString()
{
    return "$-1\r\n";
}

std::string RESP::encodeArray(const std::vector<std::string> &items)
{
    std::string result = "*" + std::to_string(items.size()) + "\r\n";
    for (const auto &item : items)
    {
        result += encodeBulkString(item);
    }
    return result;
}