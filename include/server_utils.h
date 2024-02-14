//
// Created by leo on 2/13/24.
//

#ifndef MY_REDIS_SERVER_UTILS_H
#define MY_REDIS_SERVER_UTILS_H

#include <stdlib.h>
#include <stdint.h>
#include <vector>
#include <string>

const size_t k_max_msg = 4096;

// for intrusive data structure
#define container_of(ptr, T, member) ({                  \
    const typeof( ((T *)0)->member ) *__mptr = (ptr);    \
    (T *)( (char *)__mptr - offsetof(T, member) );})

// state of incoming events
enum {
    STATE_REQ = 0,
    STATE_RES = 1,
    STATE_END = 2,
};

struct Conn {
    int fd = -1;
    uint32_t state = 0; // default as STATE_REQ
    // buffer for reading
    size_t rbuf_size = 0;
    uint8_t rbuf[4 + k_max_msg];
    // buffer for writing
    size_t wbuf_size = 0;
    size_t wbuf_sent = 0;
    uint8_t wbuf[4 + k_max_msg];
};

enum {
    RES_OK = 0,
    RES_ERR = 1,
    RES_NX = 2,
};

// structure for key-val node
struct Entry {
    struct HNode node;
    std::string key;
    std::string val;
};

// data structure for the key space
struct {
    HMap db;
} g_data;

// put a new connection state to fd2conn
void conn_put(std::vector<Conn*> &fd2conn, struct Conn *conn);

// accept a new connection and register a Struct Conn for it
int32_t accept_new_conn(std::vector<Conn*> &fd2conn, int fd, int epfd);

// state machine for client connection
void connection_io(Conn* conn, int epfd);

// handle incoming byte stream (parse with our pre-defined protocol)
bool try_one_request(Conn *conn, int epfd);




// handlers and util funcs for STATE_REQ state
void state_req(Conn *conn, int epfd); // handler
bool try_fill_buffer(Conn *conn, int epfd); // request handling means "read client msg to a buffer"

// handler and utils funcs for STATE_RES state
void state_res(Conn *conn, int epfd); // handler
bool try_flush_buffer(Conn *conn, int epfd); // response handling means "send msg in a buffer to client"

// set an fd in non-blocking mode
void fd_set_nb(int fd);

// parse the request
int32_t parse_req(const uint8_t *data, size_t len, std::vector<std::string>& out);
bool cmd_is(const std::string &word, const char *cmd);

// process the request
int32_t do_request(const uint8_t *rbuf, uint32_t request_len, uint32_t *rescode, uint8_t *wbuf, uint32_t *wlen);
uint32_t do_get(std::vector<std::string> &cmd, uint8_t *res, uint32_t *reslen);
uint32_t do_set(std::vector<std::string> &cmd, uint8_t *res, uint32_t *reslen);
uint32_t do_del(std::vector<std::string> &cmd, uint8_t *res, uint32_t *reslen);


// calculate hash value of a string
uint64_t str_hash(const uint8_t *data, size_t len);

// determine whether two keys are equal
bool entry_eq(HNode *lhs, HNode *rhs);

#endif //MY_REDIS_SERVER_UTILS_H
