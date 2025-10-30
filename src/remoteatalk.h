#ifndef REMOTEATALK_H
#define REMOTEATALK_H

#include <stdint.h>
#include <sys/socket.h>
//#include <netatalk/at.h> 

#if defined(__linux__)
#include <linux/atalk.h>
#include <netatalk/at.h>
#else
struct at_addr {
#ifdef s_net
#undef s_net
#endif /* s_net */
    unsigned short	s_net;
    unsigned char	s_node;
};
struct sockaddr_at {
    short		sat_family;
    unsigned char		sat_port;
    struct at_addr	sat_addr;
    char		sat_zero[ 8 ];
};
#endif

typedef uint16_t at_net;    /*! AppleTalk network number    */
typedef uint8_t  at_node;   /*! AppleTalk node number       */
typedef uint8_t  at_socket; /*! AppleTalk socket number     */

typedef enum atalk_socket_state_tag {
    ATALK_STATE_CLOSED,
    ATALK_STATE_OPEN
} atalk_socket_state_t;

typedef struct atalk_socket_desc_tag {
    atalk_socket_state_t state;
    int fd;
    int atalk_fd;
    struct sockaddr_at address;
} atalk_socket_desc_t;

typedef enum atalk_socket_cmd_tag {
    ATALK_CMD_OPEN,
    ATALK_CMD_RECVFROM,
    ATALK_CMD_SENDTO,
    ATALK_CMD_GETSOCKNAME,
    ATALK_CMD_CLOSE,
    ATALK_NUMBER_OF_CMDS
} atalk_socket_cmd_t;

typedef enum atalk_socket_status_tag {
    ATALK_STATUS_OK,
    ATALK_STATUS_ERROR,
    ATALK_STATUS_RESPONSE_MASK = 0x80
} atalk_socket_status_t;

int remoteatalk_netddp_open(
    atalk_socket_desc_t *desc,
    const char *interfaceName);

ssize_t remoteatalk_netddp_recvfrom(
    atalk_socket_desc_t *desc,
    void *buffer,
    size_t length,
    int flags,
    struct sockaddr *address,
    socklen_t *address_len);

ssize_t remoteatalk_netddp_sendto(
    atalk_socket_desc_t *desc,
    const void *buffer,
    size_t length,
    int flags,
    const struct sockaddr *dest_addr,
    socklen_t address_len);

int remoteatalk_netddp_close(
    atalk_socket_desc_t *desc);

int netddp_getsockname(
    atalk_socket_desc_t *desc, struct sockaddr *address,
    socklen_t *address_len);

#endif // REMOTEATALK_H