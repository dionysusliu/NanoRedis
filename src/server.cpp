//
// Created by leo on 2/12/24.
//

#include <vector>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>

#include "utils.h"
#include "server_utils.h"

#define MAX_EVENT_LEN 100

int main(){
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0){
        die("fail to create server socket");
    }
    // set server as addr reusable
    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    // bind
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(0);    // wildcard address 0.0.0.0
    int rv = bind(fd, (const sockaddr *)&addr, sizeof(addr));
    if (rv) {
        die("bind()");
    }

    // listen
    rv = listen(fd, SOMAXCONN);
    if(rv){
        die("listen() failed");
    }

    // set the listen fd to non-blocking mode
    fd_set_nb(fd);

    /* event loop */
    std::vector<Conn *> fd2conn; // map of all clients connections, keyed by fd
    std::vector<struct epoll_event> epoll_args; // record event states, data structure that comm with kernel via poll()
    int epoll_fd = epoll_create1(0);
    if(epoll_fd == -1){
        die("fail to create epoll fd!");
    }
    // epoll event buffer
    struct epoll_event events_buf[MAX_EVENT_LEN];
    // monitor the listening fd
    struct epoll_event listen_event = {};
    listen_event.events = EPOLLIN;
    listen_event.data.fd = fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &listen_event);
    while(true){
        /* prepare arguments for epoll() */
        // push all existing connection fds

        /* poll for active fds (user thread blocked */
        // kernel would mark active fds in poll_args.data() array)
        int rv = epoll_wait(epoll_fd, events_buf, MAX_EVENT_LEN, 30000);
        if(rv < 0){
            die("epoll_wait failed");
        }

        /* process active connections */
        for(size_t i = 0; i < rv; ++i){
            int active_fd = events_buf[i].data.fd;
			if (active_fd == fd){
			    (void) accept_new_conn(fd2conn, fd, epoll_fd);
				continue;
			}
            Conn *conn = fd2conn[active_fd]; // locate the buffer
            connection_io(conn, epoll_fd);
            if(conn->state == STATE_END){
                // destory the conn
                fd2conn[conn->fd] = NULL;
                (void)close(conn->fd);
				epoll_ctl(epoll_fd, EPOLL_CTL_DEL, conn->fd, NULL);
                free(conn);
            }
        }
    }

    if (close(epoll_fd)){
        die("fail to close epoll fd!");
    }

    return 0;
}




