#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>

#include "remoteatalk.h"

// int remoteatalk_netddp_open(
//     atalk_socket_desc_t *desc,
//     const char *interfaceName)
//     {

//     }

// ssize_t remoteatalk_netddp_recvfrom(
//     atalk_socket_desc_t *desc,
//     void *buffer,
//     size_t length,
//     int flags,
//     struct sockaddr *address,
//     socklen_t *address_len) 
//     {

//     }

// ssize_t remoteatalk_netddp_sendto(
//     atalk_socket_desc_t *desc,
//     const void *buffer,
//     size_t length,
//     int flags,
//     const struct sockaddr *dest_addr,
//     socklen_t address_len)
//     {

//     }

// int remoteatalk_netddp_close(
//     atalk_socket_desc_t *desc)
//     {

//     }

// int netddp_getsockname(
//     atalk_socket_desc_t *desc, struct sockaddr *address,
//     socklen_t *address_len)
//     {

//     }