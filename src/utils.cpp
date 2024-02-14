//
// Created by leo on 2/12/24.
//

#include "utils.h"
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

int32_t read_full(int fd, char *buf, size_t n){
    while(n>0){
        ssize_t rv = read(fd, buf, n);
        if(rv <= 0){
            return -1;
        }
        assert((size_t)rv <= n);
        n -= rv;
        buf += rv;
    }
    return 0;
}

int32_t write_all(int fd, const char *buf, size_t n){
    while(n>0){
        ssize_t rv = write(fd, buf, n);
        if(rv <= 0){
            return -1;
        }
        assert((size_t)rv <= n);
        n -= rv;
        buf += rv;
    }
    return 0;
}

void msg(char const *msg){
    fprintf(stderr, "%s\n", msg);
}

void die(char const *msg){
    int err = errno;
    fprintf(stderr, "[%d]: %s\n", err, msg);
    abort();
}

