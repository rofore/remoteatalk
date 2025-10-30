#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>

#include "remoteatalk.h"

#define PORT 6969
#define BUFFER_SIZE 1024

#define SOCKADDR_AT_PACKED_LENGTH 6

void handle_cmd_open(atalk_socket_desc_t *desc);
void handle_cmd_open(atalk_socket_desc_t *desc);
void handle_cmd_recvfrom(atalk_socket_desc_t *desc);
void handle_cmd_sendto(atalk_socket_desc_t *desc);
void handle_cmd_close(atalk_socket_desc_t *desc);
void handle_cmd_getsockname(atalk_socket_desc_t *desc);

int receive_atsockaddr(atalk_socket_desc_t *desc, struct sockaddr_at **addr);
int send_atsockaddr(atalk_socket_desc_t *desc, struct sockaddr_at *addr);
int send_header(atalk_socket_desc_t *desc, uint8_t command, uint8_t status);

void *handle_client(void *arg)
{
    uint8_t command;
    atalk_socket_desc_t desc;

    memset(&desc, '\0', sizeof(atalk_socket_desc_t));
    desc.state = ATALK_STATE_CLOSED;
    desc.fd = *(int *)arg;

    free(arg);

    ssize_t bytes_read;

    while ((bytes_read = read(desc.fd, &command, sizeof(command))) > 0)
    {
        if ((0 > bytes_read) || (ATALK_NUMBER_OF_CMDS <= command) || ((ATALK_STATE_CLOSED == desc.state) && (command != ATALK_CMD_OPEN)))
        {
            printf("Invalid command received: %d\n", command);
            close(desc.fd);
            return NULL;
        }

        switch (command)
        {
        case ATALK_CMD_OPEN:
            handle_cmd_open(&desc);
            break;
        case ATALK_CMD_RECVFROM:
            handle_cmd_recvfrom(&desc);
            break;
        case ATALK_CMD_SENDTO:
            handle_cmd_sendto(&desc);
            break;
        case ATALK_CMD_CLOSE:
            handle_cmd_close(&desc);
            break;
        case ATALK_CMD_GETSOCKNAME:
            handle_cmd_getsockname(&desc);
            break;
        default:
            printf("Unknown command received: %d\n", command);
            close(desc.fd);
            return NULL;
        }
    }

    close(desc.fd);
    printf("Client disconnected\n");
    return NULL;
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    int server_socket, *client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 10) < 0)
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    while (1)
    {
        client_socket = malloc(sizeof(int));
        *client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (*client_socket < 0)
        {
            perror("Accept failed");
            free(client_socket);
            continue;
        }

        printf("Connection accepted from %s:%d\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, client_socket) != 0)
        {
            perror("Thread creation failed");
            close(*client_socket);
            free(client_socket);
        }

        pthread_detach(tid); // Automatically reclaim thread resources
    }

    close(server_socket);
    return 0;
}

void handle_cmd_open(atalk_socket_desc_t *desc)
{
    assert(NULL != desc);
    assert(ATALK_STATE_CLOSED == desc->state);
    int result;
    struct sockaddr_at addr, bridge;
    struct sockaddr_at *p_addr = &addr;
    struct sockaddr_at *p_bridge = &bridge;

    int s;
    socklen_t len;

    result = 0;

    receive_atsockaddr(desc, &p_addr);
    receive_atsockaddr(desc, &p_bridge);

    if ((s = socket(AF_APPLETALK, SOCK_DGRAM, 0)) < 0)
    {
        perror("Failed to create AppleTalk socket");
        result = -1;
    }

    if (NULL != p_addr)
    {
        p_addr->sat_family = AF_APPLETALK;

        /* rest of address should be initialized by the caller */
        if (bind(s, (struct sockaddr *)p_addr, sizeof(struct sockaddr_at)) < 0)
        {
            perror("Failed to bind AppleTalk socket");
            close(s);
            result = -1;
        }

        /* get the real address from the kernel */
        len = sizeof(struct sockaddr_at);

        if (getsockname(s, (struct sockaddr *)p_addr, &len) != 0)
        {
            perror("Failed to get AppleTalk socket name");
            close(s);
            result = -1;
        }
    }

    if (0 > result)   
    {
        close(s);
        return;
    }

    desc->state = ATALK_STATE_OPEN;
    desc->atalk_fd = s;

    /* Send response */
    send_header(desc, ATALK_CMD_OPEN, (0 <= result) ? ATALK_STATUS_OK : ATALK_STATUS_ERROR);
    send_atsockaddr(desc, p_addr);
    send_atsockaddr(desc, p_bridge);
}


void handle_cmd_recvfrom(atalk_socket_desc_t *desc)
{
}

void handle_cmd_sendto(atalk_socket_desc_t *desc)
{
}

void handle_cmd_close(atalk_socket_desc_t *desc)
{
}

void handle_cmd_getsockname(atalk_socket_desc_t *desc)
{
}

int receive_atsockaddr(atalk_socket_desc_t *desc, struct sockaddr_at **addr)
{
    uint8_t length;
    int result;

    assert(NULL != desc);
    assert(NULL != addr);

    struct __attribute__((packed)) temp_addr_tag
    {
        uint16_t sat_family;
        uint8_t sat_port;
        uint16_t s_net;
        uint8_t s_node;
    } temp_addr;

    assert(sizeof(temp_addr) == SOCKADDR_AT_PACKED_LENGTH);

    result = read(desc->fd, &length, sizeof(length));

    if (result != sizeof(length))
    {
        printf("Failed to read sockaddr_at length\n");
        return -1;
    }

    if (length == SOCKADDR_AT_PACKED_LENGTH)
    {
        result = read(desc->fd, &temp_addr, SOCKADDR_AT_PACKED_LENGTH);

        if (result != SOCKADDR_AT_PACKED_LENGTH)
        {
            printf("Failed to read packed sockaddr_at\n");
            return -1;
        }
        else
        {
            (*addr)->sat_family = ntohs(temp_addr.sat_family);
            (*addr)->sat_port = temp_addr.sat_port;
            (*addr)->sat_addr.s_net = ntohs(temp_addr.s_net);
            (*addr)->sat_addr.s_node = temp_addr.s_node;
            return result;
        }
    }
    else if (0U == length)
    {
        *addr = NULL;
        return 0;
    }
    else
    {
        printf("Unsupported sockaddr_at length: %u\n", length);
        *addr = NULL;
        return -1;
    }
}

int send_header(atalk_socket_desc_t *desc, uint8_t command, uint8_t status)
{
    uint8_t header[2];
    int result;

    assert(NULL != desc);

    header[0] = command;
    header[1] = status;

    result = write(desc->fd, header, sizeof(header));

    if (result != sizeof(header))
    {
        printf("Failed to send header\n");
        return -1;
    }
    else
    {
        return result;
    }
}

int send_atsockaddr(atalk_socket_desc_t *desc, struct sockaddr_at *addr)
{
    uint8_t length;
    int result;

    assert(NULL != desc);

    if (NULL == addr)
    {
        length = 0U;
        result = write(desc->fd, &length, sizeof(length));

        if (result != sizeof(length))
        {
            printf("Failed to send zero-length sockaddr_at\n");
            return -1;
        }
        else
        {
            return result;
        }
    }
    else
    {
        length = SOCKADDR_AT_PACKED_LENGTH;
        result = write(desc->fd, &length, sizeof(length));

        if (result != sizeof(length))
        {
            printf("Failed to send sockaddr_at length\n");
            return -1;
        }

        struct __attribute__((packed)) temp_addr_tag
        {
            uint16_t sat_family;
            uint8_t sat_port;
            uint16_t s_net;
            uint8_t s_node;
        } temp_addr;

        temp_addr.sat_family = htons(addr->sat_family);
        temp_addr.sat_port = addr->sat_port;
        temp_addr.s_net = htons(addr->sat_addr.s_net);
        temp_addr.s_node = addr->sat_addr.s_node;

        result = write(desc->fd, &temp_addr, SOCKADDR_AT_PACKED_LENGTH);

        if (result != SOCKADDR_AT_PACKED_LENGTH)
        {
            printf("Failed to send packed sockaddr_at\n");
            return -1;
        }
        else
        {
            return result;
        }
    }
}