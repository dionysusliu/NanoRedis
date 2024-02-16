//
// Created by leo on 2/13/24.
//

#ifndef MY_REDIS_SERVER_UTILS_H
#define MY_REDIS_SERVER_UTILS_H

#include <stdlib.h>
#include <stdint.h>
#include <vector>
#include <string>

#include "hashtable.h"
#include "utils.h"

const size_t k_max_msg = 4096;



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




// structure for key-val node
struct Entry {
    struct HNode node;
    std::string key;
    std::string val;
};

// data structure for the key space
struct {
    struct HMap db;
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




// Data Encoding Scheme
void out_nil(std::string &out);
void out_str(std::string &out, const std::string &val);
void out_int(std::string &out, int64_t val);
void out_err(std::string &out, int32_t code, const std::string &msg);
void out_arr(std::string &out, uint32_t n);

// parse the request
int32_t parse_req(const uint8_t *data, size_t len, std::vector<std::string>& out);
bool cmd_is(const std::string &word, const char *cmd);

// process the request
void do_request(std::vector<std::string>& cmd, std::string &out);
void do_get(std::vector<std::string>& cmd, std::string &out);
void do_set(std::vector<std::string>& cmd, std::string &out);
void do_del(std::vector<std::string>& cmd, std::string &out);
void do_keys(std::vector<std::string>& cmd, std::string &out);

// calculate hash value of a string
uint64_t str_hash(const uint8_t *data, size_t len);

// determine whether two keys are equal
bool entry_eq(HNode *lhs, HNode *rhs);




// Scan callbacks
/**
 * @brief Scan callback: extract key and put in the outbuf passed through $arg
 * 
 * @param node node to be scaned
 * @param arg should be of ref type `std::string&`, used as output buf
 */
void cb_scan(HNode *node, void *arg);





#endif //MY_REDIS_SERVER_UTILS_H
