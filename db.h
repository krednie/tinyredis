#ifndef DB_H
#define DB_H

#include <unordered_map>
#include <string>
#include <fstream>
#include <chrono>
#include <vector>
#include "value.h"

class Db
{
private:
    std::unordered_map<std::string, Value> bucketstore;
    void cleanupIfExpired(const std::string &key);

    std::string rdb_filename_;
    std::string aof_filename_;
    std::ofstream aof_file_;

    std::chrono::steady_clock::time_point last_save_time_;
    int auto_save_interval_;

    void logToAOF(const std::string &command);
    void checkAutoSave();

public:

    Db(const std::string &rdb_file = "dump.json",
        const std::string& aof_file = "dump.aof",
       int auto_save_interval = 60);

    ~Db();

    // ============== String Commands ==============
    void set(const std::string &key, const std::string &value);
    bool get(const std::string &key);
    bool del(const std::string &key);
    bool exists(const std::string &key);
    bool incr(const std::string &key);
    bool incrby(const std::string &key, long long amount);
    bool decr(const std::string &key);
    bool decrby(const std::string &key, long long amount);
    long long append(const std::string& key, const std::string& value);
    long long strlen(const std::string& key);
    void mget(const std::vector<std::string>& keys);
    void mset(const std::vector<std::string>& keyvals);
    std::string getrange(const std::string& key, long long start, long long end);
    long long setrange(const std::string& key, long long offset, const std::string& value);
    
    // ============== Key Commands ==============
    bool expire(const std::string &key, long long seconds);
    long long ttl(const std::string &key);
    bool persist(const std::string &key);
    std::string type(const std::string& key);
    
    // ============== List Commands ==============
    long long lpush(const std::string& key, const std::vector<std::string>& values);
    long long rpush(const std::string& key, const std::vector<std::string>& values);
    std::string lpop(const std::string& key);
    std::string rpop(const std::string& key);
    long long llen(const std::string& key);
    std::vector<std::string> lrange(const std::string& key, long long start, long long stop);
    std::string lindex(const std::string& key, long long index);
    bool lset(const std::string& key, long long index, const std::string& value);
    
    // ============== Set Commands ==============
    long long sadd(const std::string& key, const std::vector<std::string>& members);
    long long srem(const std::string& key, const std::string& member);
    std::vector<std::string> smembers(const std::string& key);
    bool sismember(const std::string& key, const std::string& member);
    long long scard(const std::string& key);
    
    // ============== Hash Commands ==============
    bool hset(const std::string& key, const std::string& field, const std::string& value);
    std::string hget(const std::string& key, const std::string& field);
    bool hdel(const std::string& key, const std::string& field);
    std::vector<std::pair<std::string, std::string>> hgetall(const std::string& key);
    std::vector<std::string> hkeys(const std::string& key);
    std::vector<std::string> hvals(const std::string& key);
    long long hlen(const std::string& key);
    bool hexists(const std::string& key, const std::string& field);
    
    // ============== Persistence ==============
    bool saveRDB();
    bool loadRDB();
    bool loadAOF();
    void startNewAOF();
};

#endif
