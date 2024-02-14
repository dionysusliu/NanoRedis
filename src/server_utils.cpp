//
// Created by leo on 2/13/24.
//

#include "utils.h"
#include "server_utils.h"

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

static std::map<std::string, std::string> g_map;

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

int32_t do_request(
    const uint8_t *req, uint32_t reqlen, 
    uint32_t *rescode, uint8_t *res, uint32_t *reslen
){
    std::vector<std::string> cmd;
    if(0 != parse_req(req, reqlen, cmd)){
        msg("bad req");
        return -1;
    }
    if(cmd.size() == 2 && cmd_is(cmd[0], "get")){
        *rescode = do_get(cmd, res, reslen);
    } else if (cmd.size() == 3 && cmd_is(cmd[0], "set")) {
        *rescode = do_set(cmd, res, reslen);
    } else if (cmd.size() == 2 && cmd_is(cmd[0], "del")) {
        *rescode = do_del(cmd, res, reslen);
    } else {
        // cmd is not recognized
        *rescode = RES_ERR;
        const char *msg = "Unknown cmd";
        strcpy((char *)res, msg);
        *reslen = strlen(msg);
        return 0;
    }
    return 0;
}

bool try_one_request(Conn *conn, int epfd){
    // extract length info
    if (conn->rbuf_size < 4){ // length info is not complete yet
        return false;
    }
    uint32_t len = 0;
    memcpy(&len, conn->rbuf, sizeof(uint32_t));
    if(len > k_max_msg){
        msg("payload is too long");
        conn->state = STATE_END;
        return false;
    }

    // extract payload
    if(conn->rbuf_size - 4 < len){ // payload is not complete
        return false;
    }
    
    // do the request
    uint32_t rescode = 0;
    uint32_t wlen = 0;
    int32_t err = do_request(
        &conn->rbuf[4], 
        len, 
        &rescode,
        &conn->wbuf[4+4], 
        &wlen);
    if(err){
        conn->state = STATE_END;
        return false;
    }

    // fill in header and rescode in wbuf
    wlen += 4; // add 4 bytes for rescode length
    memcpy(&conn->wbuf[0], &wlen, 4);
    memcpy(&conn->wbuf[4], &rescode, 4);
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

uint32_t do_get(const std::vector<std::string> &cmd, uint8_t *res, uint32_t *reslen){
    if(!g_map.count(cmd[1])){return RES_NX;}
    
    std::string &val = g_map[cmd[1]];
    assert(val.size() <= k_max_msg);
    memcpy(res, val.data(), val.size());
    *reslen = (uint32_t)val.size();
    return RES_OK;
}

uint32_t do_set(const std::vector<std::string> &cmd, uint8_t *res, uint32_t *reslen){
    (void)res;
    (void)reslen;
    g_map[cmd[1]] = cmd[2];
    return RES_OK;
}

uint32_t do_del(const std::vector<std::string> &cmd, uint8_t *res, uint32_t *reslen){
    (void)res;
    (void)reslen;
    g_map.erase(cmd[1]);
    return RES_OK;  
}

