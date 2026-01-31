#ifndef VALUE_H
#define VALUE_H
#include <chrono>
#include <optional>
#include <string>
#include <variant>
#include <vector>
#include <deque>
#include <unordered_set>
#include <unordered_map>

enum class ValueType
{
    STRING,
    INTEGER,
    LIST,
    SET,
    HASH
};

// Type aliases for complex types
using RedisList = std::deque<std::string>;
using RedisSet = std::unordered_set<std::string>;
using RedisHash = std::unordered_map<std::string, std::string>;

struct Value
{
    ValueType type;
    
    // Data storage using variant for memory efficiency
    std::variant<
        std::string,                    // STRING
        long long,                      // INTEGER
        RedisList,                      // LIST
        RedisSet,                       // SET
        RedisHash                       // HASH
    > data;

    std::optional<std::chrono::time_point<std::chrono::steady_clock>> expiration;

    bool isExpired() const;
    void setExpiration(long long seconds);
    void persist();
    long long getTTL() const;

    // Accessors for convenience - String/Integer
    std::string& str() { return std::get<std::string>(data); }
    const std::string& str() const { return std::get<std::string>(data); }
    long long& integer() { return std::get<long long>(data); }
    long long integer() const { return std::get<long long>(data); }
    
    // Accessors for List
    RedisList& list() { return std::get<RedisList>(data); }
    const RedisList& list() const { return std::get<RedisList>(data); }
    
    // Accessors for Set
    RedisSet& set() { return std::get<RedisSet>(data); }
    const RedisSet& set() const { return std::get<RedisSet>(data); }
    
    // Accessors for Hash
    RedisHash& hash() { return std::get<RedisHash>(data); }
    const RedisHash& hash() const { return std::get<RedisHash>(data); }

    // Constructors
    Value();
    Value(const std::string &s);
    Value(long long i);
    Value(const RedisList &l);
    Value(const RedisSet &s);
    Value(const RedisHash &h);
    Value(const Value &other) = default;
    Value &operator=(const Value &other) = default;
    ~Value() = default;
};

#endif
