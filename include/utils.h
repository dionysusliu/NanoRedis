//
// Created by leo on 2/12/24.
//

#ifndef MY_REDIS_UTILS_H
#define MY_REDIS_UTILS_H

#include <stdint.h>
#include <stddef.h>

int32_t read_full(int fd, char *rbuf, size_t size);

int32_t write_all(int fd, const char *wbuf, size_t size);

void msg(char const *msg);

void die(char const *msg);

#endif //MY_REDIS_UTILS_H
