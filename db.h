#ifndef DB_H
#define DB_H

#include <unordered_map>
#include <string>
#include <fstream>
#include <chrono>
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
    bool expire(const std::string &key, long long seconds);
    long long ttl(const std::string &key);
    bool persist(const std::string &key);
    std::string type(const std::string& key);
    bool saveRDB();
    bool loadRDB();
    bool loadAOF();
    void startNewAOF();

};

#endif
#endif
