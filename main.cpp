#include "db.h"

int main() {
    Db redis;

    redis.set("user", "yash");
    redis.get("user");
    redis.exists("user");
    redis.del("user");
    redis.exists("user");

    return 0;
}
