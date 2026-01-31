#include "db.h"
#include <cstdio>
#include <iostream>
#include <vector>
#include <sstream>

static std::string jsonEscape(const std::string &s)
{
    std::string out;
    out.reserve(s.size());
    for (unsigned char c : s)
    {
        switch (c)
        {
        case '\"':
            out += "\\\"";
            break;
        case '\\':
            out += "\\\\";
            break;
        case '\b':
            out += "\\b";
            break;
        case '\f':
            out += "\\f";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            if (c < 0x20)
            {
                char buf[7];
                std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                out += buf;
            }
            else
            {
                out += static_cast<char>(c);
            }
        }
    }
    return out;
}
static std::string jsonUnescape(const std::string &s)
{
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); i++)
    {
        if (s[i] == '\\' && i + 1 < s.size())
        {
            switch (s[i + 1])
            {
            case '\"': out += '\"'; i++; break;
            case '\\': out += '\\'; i++; break;
            case 'b': out += '\b'; i++; break;
            case 'f': out += '\f'; i++; break;
            case 'n': out += '\n'; i++; break;
            case 'r': out += '\r'; i++; break;
            case 't': out += '\t'; i++; break;
            case 'u': // handle \uXXXX
                if (i + 5 < s.size())
                {
                    int code;
                    if (sscanf(s.c_str() + i + 2, "%04x", &code) == 1)
                    {
                        out += static_cast<char>(code);
                        i += 5;
                    }
                }
                break;
            default: out += s[i]; break;
            }
        }
        else
        {
            out += s[i];
        }
    }
    return out;
}

Db::Db(const std::string &rdb_file, const std::string &aof_file, int auto_save_interval)
    : rdb_filename_(rdb_file), aof_filename_(aof_file), auto_save_interval_(auto_save_interval)
{
    loadRDB();
    loadAOF();

    aof_file_.open(aof_filename_, std::ios::app);
    last_save_time_ = std::chrono::steady_clock::now();

    std::cout << "# Database loaded from " << rdb_filename_
              << " and " << aof_filename_ << std::endl;
}

Db::~Db()
{
    aof_file_.close();
    saveRDB();

    std::cout << "# Database saved to " << rdb_filename_ << std::endl;
}

void Db::logToAOF(const std::string &command)
{
    if (aof_file_.is_open())
    {
        aof_file_ << command << std::endl;
        aof_file_.flush();
    }
}

void Db::checkAutoSave()
{
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_save_time_).count();

    if (elapsed >= auto_save_interval_)
    {
        std::cout << "# Auto-saving..." << std::endl;
        saveRDB();
        startNewAOF();
        last_save_time_ = now;
    }
}

bool Db::saveRDB()
{
    std::ofstream file(rdb_filename_);
    if (!file.is_open())
    {
        std::cout << "(error) Failed to open RDB file for writing" << std::endl;
        return false;
    }

    file << "{\n";

    bool first = true;

    for (const auto &pair : bucketstore)
    {
        const std::string &key = pair.first;
        const Value &val = pair.second;

        if (val.isExpired())
        {
            continue;
        }

        if (!first)
        {
            file << ",\n";
        }
        first = false;

        file << "  \"" << key << "\": {\n";

        if (val.type == ValueType::STRING)
        {
            file << "    \"type\": \"string\",\n";
            file << "    \"value\": \"" << jsonEscape(val.str()) << "\"";
        }
        else if (val.type == ValueType::INTEGER)
        {
            file << "    \"type\": \"integer\",\n";
            file << "    \"value\": " << val.integer();
        }

        long long ttl = val.getTTL();
        if (ttl >= 0)
        {
            file << ",\n    \"ttl\": " << ttl;
        }
        else
        {
            file << ",\n    \"ttl\": -1";
        }

        file << "\n  }";
    }

    file << "\n}\n";
    file.close();

    std::cout << "OK" << std::endl;
    return true;
}

bool Db::loadRDB()
{
    std::ifstream file(rdb_filename_);
    if (!file.is_open())
    {
        return false;
    }

    std::string line;
    std::string currentKey;
    std::string valueType;
    std::string valueStr;
    long long valueInt = 0;
    long long ttl = -1;

    while (std::getline(file, line))
    {

        size_t start = line.find_first_not_of(" \t");
        size_t end = line.find_last_not_of(" \t,");
        if (start == std::string::npos)
            continue;
        line = line.substr(start, end - start + 1);

        if (line == "{" || line == "}" || line.empty())
            continue;

        if (line.find("\":") != std::string::npos && line.back() == '{')
        {
            size_t keyStart = line.find('"') + 1;
            size_t keyEnd = line.find('"', keyStart);
            currentKey = line.substr(keyStart, keyEnd - keyStart);
            continue;
        }

        if (line.find("\"type\":") != std::string::npos)
        {
            size_t valStart = line.find(": \"") + 3;
            size_t valEnd = line.find('"', valStart);
            valueType = line.substr(valStart, valEnd - valStart);
            continue;
        }

        if (line.find("\"value\":") != std::string::npos)
        {
            if (valueType == "string")
            {
                size_t valStart = line.find(": \"") + 3;
                size_t valEnd = line.find('"', valStart);
                valueStr = jsonUnescape(line.substr(valStart, valEnd - valStart));
            }
            else if (valueType == "integer")
            {
                size_t valStart = line.find(": ") + 2;
                valueInt = std::stoll(line.substr(valStart));
            }
            continue;
        }

        if (line.find("\"ttl\":") != std::string::npos)
        {
            size_t valStart = line.find(": ") + 2;
            std::string ttlStr = line.substr(valStart);
            if (ttlStr != "null" && ttlStr != "-1")
            {
                ttl = std::stoll(ttlStr);
            }
            else
            {
                ttl = -1;
            }

            if (!currentKey.empty())
            {
                Value v;
                if (valueType == "string")
                {
                    v = Value(valueStr);
                }
                else if (valueType == "integer")
                {
                    v = Value(valueInt);
                }

                if (ttl > 0)
                {
                    v.setExpiration(ttl);
                }

                bucketstore[currentKey] = v;

                currentKey.clear();
                valueType.clear();
                valueStr.clear();
                valueInt = 0;
                ttl = -1;
            }
        }
    }

    file.close();
    return true;
}

bool Db::loadAOF()
{
    std::ifstream file(aof_filename_);
    if (!file.is_open())
    {
        return false; // no aof yet
    }

    std::string line;
    int commands_replayed = 0;

    while (std::getline(file, line))
    {
        if (line.empty())
            continue;

        std::istringstream iss(line);
        std::vector<std::string> tokens;
        std::string word;

        while (iss >> word)
            tokens.push_back(word);

        if (tokens.empty())
            continue;

        const std::string &cmd = tokens[0];

        // --------------------------------------------------------------------
        // SET key value...
        // --------------------------------------------------------------------
        if (cmd == "SET" && tokens.size() >= 3)
        {
            std::string key = tokens[1];
            std::string value;

            for (size_t i = 2; i < tokens.size(); i++)
            {
                if (i > 2)
                    value += " ";
                value += tokens[i];
            }

            bucketstore[key] = Value(value);
        }

        // --------------------------------------------------------------------
        // DEL key
        // --------------------------------------------------------------------
        else if (cmd == "DEL" && tokens.size() >= 2)
        {
            bucketstore.erase(tokens[1]);
        }

        // --------------------------------------------------------------------
        // INCR key
        // --------------------------------------------------------------------
        else if (cmd == "INCR" && tokens.size() >= 2)
        {
            auto it = bucketstore.find(tokens[1]);

            if (it == bucketstore.end())
            {
                bucketstore[tokens[1]] = Value(1);
            }
            else if (it->second.type == ValueType::INTEGER)
            {
                it->second.integer() += 1;
            }
        }

        // --------------------------------------------------------------------
        // INCRBY key amount
        // --------------------------------------------------------------------
        else if (cmd == "INCRBY" && tokens.size() >= 3)
        {
            long long amount = std::stoll(tokens[2]);
            auto it = bucketstore.find(tokens[1]);

            if (it == bucketstore.end())
            {
                bucketstore[tokens[1]] = Value(amount);
            }
            else if (it->second.type == ValueType::INTEGER)
            {
                it->second.integer() += amount;
            }
        }

        // --------------------------------------------------------------------
        // DECR key
        // --------------------------------------------------------------------
        else if (cmd == "DECR" && tokens.size() >= 2)
        {
            auto it = bucketstore.find(tokens[1]);

            if (it == bucketstore.end())
            {
                bucketstore[tokens[1]] = Value(-1);
            }
            else if (it->second.type == ValueType::INTEGER)
            {
                it->second.integer() -= 1;
            }
        }

        // --------------------------------------------------------------------
        // DECRBY key amount
        // --------------------------------------------------------------------
        else if (cmd == "DECRBY" && tokens.size() >= 3)
        {
            long long amount = std::stoll(tokens[2]);
            auto it = bucketstore.find(tokens[1]);

            if (it == bucketstore.end())
            {
                bucketstore[tokens[1]] = Value(-amount);
            }
            else if (it->second.type == ValueType::INTEGER)
            {
                it->second.integer() -= amount;
            }
        }

        // --------------------------------------------------------------------
        // APPEND key value
        // --------------------------------------------------------------------
        else if (cmd == "APPEND" && tokens.size() >= 3)
        {
            auto it = bucketstore.find(tokens[1]);
            if (it == bucketstore.end())
            {
                bucketstore[tokens[1]] = Value(tokens[2]);
            }
            else if (it->second.type == ValueType::STRING)
            {
                it->second.str().append(tokens[2]);
            }
        }

        // --------------------------------------------------------------------
        // SETRANGE key offset value
        // --------------------------------------------------------------------
        else if (cmd == "SETRANGE" && tokens.size() >= 4)
        {
            long long offset = std::stoll(tokens[2]);
            const std::string &value = tokens[3];
            auto it = bucketstore.find(tokens[1]);

            if (it == bucketstore.end())
            {
                std::string newStr(offset + value.length(), '\0');
                for (size_t i = 0; i < value.length(); i++)
                {
                    newStr[offset + i] = value[i];
                }
                bucketstore[tokens[1]] = Value(newStr);
            }
            else if (it->second.type == ValueType::STRING)
            {
                if (offset + value.length() > it->second.str().length())
                {
                    it->second.str().resize(offset + value.length(), '\0');
                }
                for (size_t i = 0; i < value.length(); i++)
                {
                    it->second.str()[offset + i] = value[i];
                }
            }
        }

        // --------------------------------------------------------------------
        // EXPIRE key seconds
        // --------------------------------------------------------------------
        else if (cmd == "EXPIRE" && tokens.size() >= 3)
        {
            auto it = bucketstore.find(tokens[1]);
            if (it != bucketstore.end())
            {
                long long seconds = std::stoll(tokens[2]);
                it->second.setExpiration(seconds);
            }
        }

        // --------------------------------------------------------------------
        // PERSIST key
        // --------------------------------------------------------------------
        else if (cmd == "PERSIST" && tokens.size() >= 2)
        {
            auto it = bucketstore.find(tokens[1]);
            if (it != bucketstore.end())
            {
                it->second.persist();
            }
        }

        commands_replayed++;
    }

    file.close();

    if (commands_replayed > 0)
        std::cout << "# Replayed " << commands_replayed << " commands from AOF" << std::endl;

    return true;
}

void Db::startNewAOF()
{

    if (aof_file_.is_open())
    {
        aof_file_.close();
    }

    std::remove(aof_filename_.c_str());

    aof_file_.open(aof_filename_, std::ios::app);

    std::cout << "# Started new AOF" << std::endl;
}

void Db::set(const std::string &key, const std::string &value)
{
    bucketstore[key] = Value(value);
    logToAOF("SET " + key + " " + value);
    checkAutoSave();
    std::cout << "OK" << std::endl;
}

bool Db::get(const std::string &key)
{
    cleanupIfExpired(key);
    auto it = bucketstore.find(key);
    if (it == bucketstore.end())
    {
        std::cout << "nil" << std::endl;
        return false;
    }

    Value &v = it->second;

    if (v.type == ValueType::STRING)
    {
        std::cout << v.str() << std::endl;
        return true;
    }
    else if (v.type == ValueType::INTEGER)
    {
        std::cout << v.integer() << std::endl;
        return true;
    }
    return false;
}

bool Db::del(const std::string &key)
{
    cleanupIfExpired(key);
    auto it = bucketstore.find(key);
    if (it == bucketstore.end())
    {
        std::cout << "(integer) 0" << std::endl;
        return false;
    }

    bucketstore.erase(it);

    logToAOF("DEL " + key);
    checkAutoSave();
    std::cout << "(integer) 1" << std::endl;
    return true;
}

bool Db::exists(const std::string &key)
{
    cleanupIfExpired(key);
    if (bucketstore.find(key) == bucketstore.end())
    {
        std::cout << "(integer) 0" << std::endl;
        return false;
    }

    std::cout << "(integer) 1" << std::endl;
    return true;
}

bool Db::incr(const std::string &key)
{
    cleanupIfExpired(key);
    auto it = bucketstore.find(key);

    if (it == bucketstore.end())
    {
        bucketstore[key] = Value(1);
        std::cout << "(integer) 1" << std::endl;
        logToAOF("INCR " + key);
        checkAutoSave();
        return true;
    }

    Value &v = it->second;

    if (v.type != ValueType::INTEGER)
    {
        std::cout << "(error) WRONGTYPE" << std::endl;
        return false;
    }

    v.integer() += 1;
    std::cout << "(integer) " << v.integer() << std::endl;
    logToAOF("INCR " + key);
    checkAutoSave();

    return true;
}

bool Db::incrby(const std::string &key, long long amount)
{
    cleanupIfExpired(key);
    auto it = bucketstore.find(key);

    if (it == bucketstore.end())
    {
        bucketstore[key] = Value(amount);
        logToAOF("INCRBY " + key + " " + std::to_string(amount));
        checkAutoSave();

        std::cout << "(integer) " << amount << std::endl;
        return true;
    }

    Value &v = it->second;

    if (v.type != ValueType::INTEGER)
    {
        std::cout << "(error) WRONGTYPE" << std::endl;
        return false;
    }

    v.integer() += amount;
    logToAOF("INCRBY " + key + " " + std::to_string(amount));
    checkAutoSave();
    std::cout << "(integer) " << v.integer() << std::endl;
    return true;
}

bool Db::decr(const std::string &key)
{

    return incrby(key, -1);
}
bool Db::decrby(const std::string &key, long long amount)
{

    return incrby(key, -amount);
}

std::string Db::type(const std::string &key)
{
    cleanupIfExpired(key);
    auto it = bucketstore.find(key);
    if (it == bucketstore.end())
    {
        std::cout << "none" << std::endl;
        return "none";
    }

    Value &v = it->second;

    if (v.type == ValueType::STRING)
    {
        std::cout << "string" << std::endl;
        return "string";
    }

    if (v.type == ValueType::INTEGER)
    {
        std::cout << "integer" << std::endl;
        return "integer";
    }

    return "none";
}

void Db::cleanupIfExpired(const std::string &key)
{
    auto it = bucketstore.find(key);
    if (it != bucketstore.end() && it->second.isExpired())
    {
        bucketstore.erase(it);
    }
}

bool Db::expire(const std::string &key, long long seconds)
{
    cleanupIfExpired(key);

    auto it = bucketstore.find(key);
    if (it == bucketstore.end())
    {
        std::cout << "(integer) 0" << std::endl;
        return false;
    }

    it->second.setExpiration(seconds);
    logToAOF("EXPIRE " + key + " " + std::to_string(seconds));
    checkAutoSave();
    std::cout << "(integer) 1" << std::endl;
    return true;
}

long long Db::ttl(const std::string &key)
{
    cleanupIfExpired(key);

    auto it = bucketstore.find(key);
    if (it == bucketstore.end())
    {
        std::cout << "(integer) -2" << std::endl;
        return -2;
    }

    long long ttl_value = it->second.getTTL();
    std::cout << "(integer) " << ttl_value << std::endl;
    return ttl_value;
}

bool Db::persist(const std::string &key)
{
    cleanupIfExpired(key);

    auto it = bucketstore.find(key);
    if (it == bucketstore.end())
    {
        std::cout << "(integer) 0" << std::endl;
        return false;
    }
    if (it->second.getTTL() == -1)

    {
        std::cout << "(integer) 0" << std::endl;
        return false;
    }

    it->second.persist();
    logToAOF("PERSIST " + key);
    checkAutoSave();
    std::cout << "(integer) 1" << std::endl;
    return true;
}

long long Db::append(const std::string &key, const std::string &value)
{
    cleanupIfExpired(key);
    auto it = bucketstore.find(key);

    if (it == bucketstore.end())
    {
        bucketstore[key] = Value(value);
        logToAOF("APPEND " + key + " " + value);
        checkAutoSave();

        std::cout << "(integer) " << value.length() << std::endl;
        return value.length();
    }

    Value &v = it->second;

    if (v.type != ValueType::STRING)
    {
        std::cout << "(error) WRONGTYPE" << std::endl;
        return -1;
    }

    v.str().append(value);
    logToAOF("APPEND " + key + " " + value);
    checkAutoSave();
    std::cout << "(integer) " << v.str().length() << std::endl;
    return v.str().length();
}

long long Db::strlen(const std::string &key)
{
    cleanupIfExpired(key);

    auto it = bucketstore.find(key);

    if (it == bucketstore.end())
    {
        std::cout << "(integer) 0" << std::endl;
        return 0;
    }

    Value &v = it->second;

    if (v.type != ValueType::STRING)
    {
        std::cout << "(error) WRONGTYPE" << std::endl;
        return -1;
    }

    std::cout << "(integer) " << v.str().length() << std::endl;
    return v.str().length();
}

void Db::mget(const std::vector<std::string> &keys)
{
    for (size_t i = 0; i < keys.size(); i++)
    {
        cleanupIfExpired(keys[i]);

        auto it = bucketstore.find(keys[i]);

        std::cout << (i + 1) << ") ";

        if (it == bucketstore.end())
        {
            std::cout << "(nil)" << std::endl;
            continue;
        }

        Value &v = it->second;

        if (v.type == ValueType::STRING)
        {
            std::cout << "\"" << v.str() << "\"" << std::endl;
        }
        else if (v.type == ValueType::INTEGER)
        {
            std::cout << v.integer() << std::endl;
        }
        else
        {
            std::cout << "(nil)" << std::endl;
        }
    }
}

void Db::mset(const std::vector<std::string> &keyvals)
{
    if (keyvals.size() % 2 != 0)
    {
        std::cout << "(error) wrong number of arguments" << std::endl;
        return;
    }

    for (size_t i = 0; i < keyvals.size(); i += 2)
    {
        const std::string &key = keyvals[i];
        const std::string &value = keyvals[i + 1];

        bucketstore[key] = Value(value);

        logToAOF("SET " + key + " " + value);
        checkAutoSave();
    }

    std::cout << "OK" << std::endl;
}

std::string Db::getrange(const std::string &key, long long start, long long end)
{
    cleanupIfExpired(key);

    auto it = bucketstore.find(key);
    if (it == bucketstore.end())
    {
        std::cout << "\"\"" << std::endl;
        return "";
    }
    Value &v = it->second;

    if (v.type != ValueType::STRING)
    {
        std::cout << "(error) WRONGTYPE" << std::endl;
        return "";
    }

    long long len = v.str().length();

    if (start < 0)
        start = len + start;
    if (end < 0)
        end = len + end;
    if (start < 0)
        start = 0;
    if (end >= len)
        end = len - 1;
    if (start > end || start >= len)
    {
        std::cout << "\"\"" << std::endl;
        return "";
    }

    std::string result = v.str().substr(start, end - start + 1);
    std::cout << "\"" << result << "\"" << std::endl;
    return result;
}

long long Db::setrange(const std::string &key, long long offset, const std::string &value)
{
    cleanupIfExpired(key);

    auto it = bucketstore.find(key);

    if (it == bucketstore.end())
    {
        std::string newStr(offset + value.length(), '\0');
        for (size_t i = 0; i < value.length(); i++)
        {
            newStr[offset + i] = value[i];
        }
        bucketstore[key] = Value(newStr);
        logToAOF("SETRANGE " + key + " " + std::to_string(offset) + " " + value);
        checkAutoSave();
        std::cout << "(integer) " << newStr.length() << std::endl;
        return newStr.length();
    }

    Value &v = it->second;

    if (v.type != ValueType::STRING)
    {
        std::cout << "(error) WRONGTYPE" << std::endl;
        return -1;
    }

    if (offset + value.length() > v.str().length())
    {
        v.str().resize(offset + value.length(), '\0');
    }

    for (size_t i = 0; i < value.length(); i++)
    {
        v.str()[offset + i] = value[i];
    }
    logToAOF("SETRANGE " + key + " " + std::to_string(offset) + " " + value);
    checkAutoSave();

    std::cout << "(integer) " << v.str().length() << std::endl;
    return v.str().length();
}
