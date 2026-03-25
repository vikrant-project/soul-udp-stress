#include "socket_wrapper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

int socket_wrapper_init(void) {
    return 0;
}

void socket_wrapper_cleanup(void) {
}

socket_t my_socket_create(void) {
    socket_t sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        perror("socket creation failed");
        return INVALID_SOCKET;
    }
    return sock;
}

int my_socket_configure(socket_t sock) {
    int optval = 1;
    
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        perror("setsockopt SO_REUSEADDR failed");
        return -1;
    }
    
#ifdef SO_REUSEPORT
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) < 0) {
        perror("setsockopt SO_REUSEPORT failed");
    }
#endif
    
    int sendbuf = 4 * 1024 * 1024;
    if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &sendbuf, sizeof(sendbuf)) < 0) {
        perror("setsockopt SO_SNDBUF failed");
    }
    
#ifdef IP_TOS
    int tos = IPTOS_THROUGHPUT;
    if (setsockopt(sock, IPPROTO_IP, IP_TOS, &tos, sizeof(tos)) < 0) {
        perror("setsockopt IP_TOS failed");
    }
#endif
    
    return 0;
}

int my_socket_bind_any(socket_t sock) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = 0;
    
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind failed");
        return -1;
    }
    
    return 0;
}

int my_socket_connect(socket_t sock, const char *ip, uint16_t port) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, ip, &addr.sin_addr) != 1) {
        fprintf(stderr, "Invalid IP address: %s\n", ip);
        return -1;
    }
    
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect failed");
        return -1;
    }
    
    return 0;
}

ssize_t my_socket_send(socket_t sock, const void *data, size_t len) {
    ssize_t sent = send(sock, data, len, MSG_DONTWAIT);
    if (sent < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != ECONNREFUSED) {
        perror("send failed");
    }
    return sent;
}

ssize_t my_socket_sendto(socket_t sock, const void *data, size_t len, 
                         const char *ip, uint16_t port) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, ip, &addr.sin_addr) != 1) {
        return -1;
    }
    
    ssize_t sent = sendto(sock, data, len, MSG_DONTWAIT,
                          (struct sockaddr *)&addr, sizeof(addr));
    if (sent < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        perror("sendto failed");
    }
    return sent;
}

int my_socket_close(socket_t sock) {
    return close(sock);
}

int my_socket_set_nonblock(socket_t sock) {
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags < 0) {
        perror("fcntl F_GETFL failed");
        return -1;
    }
    
    if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("fcntl F_SETFL failed");
        return -1;
    }
    
    return 0;
}

int my_socket_get_sendbuf_size(socket_t sock) {
    int size = 0;
    socklen_t len = sizeof(size);
    if (getsockopt(sock, SOL_SOCKET, SO_SNDBUF, &size, &len) < 0) {
        perror("getsockopt SO_SNDBUF failed");
        return -1;
    }
    return size;
}

int my_socket_set_sendbuf_size(socket_t sock, int size) {
    if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size)) < 0) {
        perror("setsockopt SO_SNDBUF failed");
        return -1;
    }
    return 0;
}
