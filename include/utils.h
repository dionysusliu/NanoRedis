//
// Created by leo on 2/12/24.
//

#ifndef MY_REDIS_UTILS_H
#define MY_REDIS_UTILS_H

// #define VERBOSE

#include <stdint.h>
#include <stddef.h>

/**
 * @brief data types for serialization
 * 
 */
enum {
    SER_NIL = 0,    // Like `NULL`
    SER_ERR = 1,    // error code
    SER_STR = 2,    // string
    SER_INT = 3,    // int64
    SER_ARR = 4,    // Array
};

/**
 * @brief error codes for out_err()
 * 
 */
enum {
    ERR_2BIG = 0,
    ERR_UNKNOWN = 1,
};

// for intrusive data structure
#define container_of(ptr, T, member) ({                  \
    const typeof( ((T *)0)->member ) *__mptr = (ptr);    \
    (T *)( (char *)__mptr - offsetof(T, member) );})

int32_t read_full(int fd, char *rbuf, size_t size);

int32_t write_all(int fd, const char *wbuf, size_t size);

void msg(char const *msg);

void die(char const *msg);

uint64_t str_hash(const uint8_t *data, size_t len);

#endif //MY_REDIS_UTILS_H
