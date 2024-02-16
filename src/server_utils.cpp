//
// Created by leo on 2/13/24.
//

#include "utils.h"
#include "server_utils.h"
#include "hashtable.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cassert>
#include <errno.h>
#include <cstring>
#include <stdio.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <string>
#include <map>
#include <iostream>


void fd_set_nb(int fd){
    errno = 0;
    int flags = fcntl(fd, F_GETFL, 0);
    if(errno){
        die("fcntl error");
        return;
    }

    flags |= O_NONBLOCK; // set as non-blocking

    errno = 0;
    (void)fcntl(fd, F_SETFL, flags);
    if(errno){
        die("fcntl error");
    }
}

void conn_put(std::vector<Conn*> &fd2conn, struct Conn *conn){
    if(fd2conn.size() <= (size_t)conn->fd){
        fd2conn.resize(conn->fd + 1);
    }
    fd2conn[conn->fd] = conn;
}

int32_t accept_new_conn(std::vector<Conn*> &fd2conn, int fd, int epfd){
    // accept a client connection request
    struct sockaddr_in client_addr = {};
    socklen_t socklen = sizeof(client_addr);
    int connfd = accept(fd, (struct sockaddr *)&client_addr, &socklen);
    if (connfd < 0) {
        msg("accept() error");
        return -1;  // error
    }

    // set new connection fd to non-blocking mode
    fd_set_nb(connfd);

    // creating the Conn struct as state of this server-client connection
    struct Conn *conn = (struct Conn *)malloc(sizeof(struct Conn));
    if(!conn){
        close(connfd);
        return -1;
    }
    conn->fd = connfd;
    conn->state = STATE_REQ;
    conn->rbuf_size = 0;
    conn->wbuf_size = 0;
    conn->wbuf_sent = 0;
    conn_put(fd2conn, conn);

	// epoll should monitor connfd
	struct epoll_event conn_event;
	conn_event.events = EPOLLIN;
	conn_event.data.fd = connfd;
	int rv = epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &conn_event);
	if (rv){
		die("epoll_ctl failed");
	}

    return 0;
}

void connection_io(Conn *conn, int epfd){
    if (conn->state == STATE_REQ){
        state_req(conn, epfd);
    } else if (conn->state == STATE_RES){
        state_res(conn, epfd);
    } else {
        assert(0); // not expected
    }
}

void do_request(std::vector<std::string> &cmd, std::string &out){
    if (cmd.size() == 1 && cmd_is(cmd[0], "keys")) {
        do_keys(cmd, out);
    } else if(cmd.size() == 2 && cmd_is(cmd[0], "get")) {
        do_get(cmd, out);
    } else if(cmd.size() == 3 && cmd_is(cmd[0], "set")) {
        do_set(cmd, out);
    } else if(cmd.size() == 2 && cmd_is(cmd[0], "del")) {
        do_del(cmd, out);
    } else { // cmd is not recognized
        out_err(out, ERR_UNKNOWN, "Unknown cmd");
    }
}

bool try_one_request(Conn *conn, int epfd){
    // extract length info
    if (conn->rbuf_size < 4){ // length info is not complete yet
        return false;
    }
    uint32_t len = 0;
    memcpy(&len, &conn->rbuf[0], sizeof(uint32_t));
    if(len > k_max_msg){
        msg("payload is too long");
        conn->state = STATE_END;
        return false;
    }

    // extract payload
    if(conn->rbuf_size - 4 < len){ // payload is not complete
        return false;
    }

    // parse the request
    std::vector<std::string> cmd;
    if(0 != parse_req(&conn->rbuf[4], len, cmd)) {
        msg("bad msg");
        conn->state = STATE_END;
        return false;
    }
    
    // do the request
    std::string out;
    do_request(cmd, out);

    // now pack the response into the buffer
    if(4 + out.size() > k_max_msg) {
        out.clear();
        out_err(out, ERR_2BIG, "response is too big");
    }

    // fill in header and rescode in wbuf
    uint32_t wlen = (uint32_t)out.size(); // add 4 bytes for rescode length
    memcpy(&conn->wbuf[0], &wlen, 4);
    memcpy(&conn->wbuf[4], out.data(), out.size());
    conn->wbuf_size = 4 + wlen;

    // remove this request from rbuf
    // note: frequent removal is inefficient
    size_t remain = conn->rbuf_size - (4 + len);
    if (remain){
        memmove(conn->rbuf, &conn->rbuf[4+len], remain);
    }
    conn->rbuf_size = remain;

    // update state of connection as RES, and the event type for epoll monitor, then respond to the client
    conn->state = STATE_RES;

	struct epoll_event epollout_event = {};
	epollout_event.events = EPOLLOUT;
	epollout_event.data.fd = conn->fd;
	epoll_ctl(epfd, EPOLL_CTL_MOD, conn->fd, &epollout_event);

	state_res(conn, epfd);

    // continue the outer loop if all requests are processed
    return (conn->state == STATE_REQ);
}

void state_req(Conn *conn, int epfd){
    while(try_fill_buffer(conn, epfd)){}
}

bool try_fill_buffer(Conn *conn, int epfd){
    // try to fill the read buffer
    assert(conn->rbuf_size < sizeof(conn->rbuf));
    ssize_t rv = 0;
    do{
        size_t cap = sizeof(conn->rbuf) - conn->rbuf_size;
        rv = read(conn->fd, &conn->rbuf[conn->rbuf_size], cap);
    }while(rv < 0 && errno == EINTR); // retry if read failed because of system interrupts

    // error handling
    if (rv < 0 && errno == EAGAIN){ // EAGAIN: no data available right now, try AGAIN later
        return false;
    }
    if (rv < 0){ // socket read failure, don't wait for recovery, just close conn
        msg("read() error");
        conn->state = STATE_END;
        return false;
    }
    if (rv == 0){ // EOF received
        if (conn->rbuf_size > 0){
            msg("unexpected EOF");
        }else{
            msg("EOF");
        }
        conn->state = STATE_END;
        return false;
    }

    // update rbuf states
    conn->rbuf_size += (size_t)rv;
    assert(conn->rbuf_size < (size_t)sizeof(conn->rbuf));

    // try to process requests one by one
    while(try_one_request(conn, epfd)){}
    return (conn->state == STATE_REQ);
}

void state_res(Conn *conn, int epfd){
    while(try_flush_buffer(conn, epfd)){}
}

bool try_flush_buffer(Conn *conn, int epfd){
    // send bytes to client
    ssize_t rv = 0;
    do {
        size_t remain = conn->wbuf_size - conn->wbuf_sent;
        rv = write(conn->fd, &conn->wbuf[conn->wbuf_sent], remain);
    }while(rv < 0 && errno == EINTR);

    // error handling
    if (rv < 0 && errno == EAGAIN) {
        // got EAGAIN, stop.
        return false;
    }
    if (rv < 0) {
        msg("write() error");
        conn->state = STATE_END;
        return false;
    }

    // update connection states
    conn->wbuf_sent += (size_t)rv;
    assert(conn->wbuf_sent <= conn->wbuf_size);
    if(conn->wbuf_sent == conn->wbuf_size){ // response is fully sent
		// conn state
        conn->state = STATE_REQ;
        conn->wbuf_sent = 0;
        conn->wbuf_size = 0;

		// event type to for epoll monitor
	    struct epoll_event epollin_event = {};
	    epollin_event.events = EPOLLIN;
	    epollin_event.data.fd = conn->fd;
	    epoll_ctl(epfd, EPOLL_CTL_MOD, conn->fd, &epollin_event);
        return false;
    }
    // still got data in wbuf, try to write again
    return true;
}



int32_t parse_req(const uint8_t *data, size_t len, std::vector<std::string>& out){
    if(len < 4) return -1;

    uint32_t n = 0;
    memcpy(&n, data, 4);
    if(n > k_max_msg) return -1;

    size_t pos = 4;
    while(n--){
        if(pos + 4 > len) return -1;
        
        uint32_t sz = 0;
        memcpy(&sz, &data[pos], 4);
        if(pos + 4 + sz > len) return -1;

        out.push_back(std::string((char *)&data[pos+4], sz));
        pos += 4 + sz;
    }

    if (pos != len){
        return -1; // trailing garbage
    }
    return 0;
}

bool cmd_is(const std::string &word, const char *cmd){
    return 0 == strcasecmp(word.c_str(), cmd);
}






void do_keys(std::vector<std::string>& cmd, std::string &out){
    (void)cmd;
    out_arr(out, (uint32_t)hm_size(&g_data.db));
    h_scan(&g_data.db.tb1, &cb_scan, &out);
    h_scan(&g_data.db.tb2, &cb_scan, &out);
}




void do_get(std::vector<std::string>& cmd, std::string &out){
    Entry key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());

    HNode *node = hm_lookup(&g_data.db, &key.node, &entry_eq);
    if(!node){ // not exist
        out_nil(out);
        return;
    }

    // fetch the data
    const std::string &val = container_of(node, Entry, node)->val; assert(val.size() < k_max_msg);
    out_str(out, val);
}


void do_set(std::vector<std::string>& cmd, std::string &out){
    Entry key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());
    HNode *query = hm_lookup(&g_data.db, &key.node, entry_eq);
    if(query){ // key already exist
        container_of(query, Entry, node)->val.swap(cmd[2]);
    } else { // key not found
        Entry *new_entry = (Entry *)malloc(sizeof(Entry)); // heap allocation
        new_entry->key.swap(key.key);
        new_entry->val.swap(cmd[2]);
        new_entry->node.hcode = str_hash((uint8_t *)new_entry->key.data(), new_entry->key.size());
        hm_insert(&g_data.db, &new_entry->node);
    }
    out_nil(out);
}


void do_del(std::vector<std::string>& cmd, std::string &out){
    // deletion 
    // 1. construct key for query
    Entry key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());
    // 2. get the Entry object on heap (if exist)
    HNode *del_node = hm_pop(&g_data.db, &key.node, entry_eq);
    if (del_node){
        free((Entry *)container_of(del_node, Entry, node)); // heap deallocation
    }

    // respond with an interger, indicating whether the deletion took place
    return out_int(out, del_node ? 1:0);
}



bool entry_eq(HNode *lhs, HNode *rhs){
    msg("entry_eq()");
    struct Entry *le = container_of(lhs, struct Entry, node);
    msg("   got lhs entry");
    struct Entry *re = container_of(rhs, struct Entry, node);
    msg("   got rhs entry");
    return le->key == re->key;
}



// Data Encoding Scheme
void out_nil(std::string &out){
    // 1b NIL SIGNAL
    out.push_back(SER_NIL);
}

void out_str(std::string &out, const std::string &val){
    // 1b - SER_STR
    // 4b - msg length
    // varlen - msg
    out.push_back(SER_STR);
    uint32_t len = (uint32_t)val.size();
    out.append((char *)&len, 4);
    out.append(val);
}

void out_int(std::string &out, int64_t val){
    // 1b - SER_INT
    // 4b - val
    out.push_back(SER_INT);
    out.append((char *)&val, 8);
}

void out_err(std::string &out, int32_t code, const std::string &msg){
    // 1b - SER_ERR
    // 4b - ERR_CODE
    // 4b - msg length
    // varlen - msg
    out.push_back(SER_ERR);
    out.append((char *)&code, 4);
    uint32_t msg_len = (uint32_t)out.size();
    out.append((char *)&msg_len, 4);
    out.append(msg);
}

void out_arr(std::string &out, uint32_t n) {
    // 1b - SER_ARR
    // 4b - array length
    out.push_back(SER_ARR);
    out.append((char *)&n, 4);
}




// Scan Callbacks
void cb_scan(HNode *node, void *arg){
    std::string &out = *(std::string *)arg;
    out_str(out, container_of(node, Entry, node)->key);
}
