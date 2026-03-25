#ifndef SOCKET_WRAPPER_H
#define SOCKET_WRAPPER_H

#include <stdint.h>
#include <sys/types.h>

typedef int socket_t;

#define INVALID_SOCKET -1

int socket_wrapper_init(void);
void socket_wrapper_cleanup(void);

socket_t my_socket_create(void);
int my_socket_configure(socket_t sock);
int my_socket_bind_any(socket_t sock);
int my_socket_connect(socket_t sock, const char *ip, uint16_t port);
ssize_t my_socket_send(socket_t sock, const void *data, size_t len);
ssize_t my_socket_sendto(socket_t sock, const void *data, size_t len, const char *ip, uint16_t port);
int my_socket_close(socket_t sock);

int my_socket_set_nonblock(socket_t sock);
int my_socket_get_sendbuf_size(socket_t sock);
int my_socket_set_sendbuf_size(socket_t sock, int size);

#endif
