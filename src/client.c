/*---------------------------------------------------------------------------*/
/* client.c                                                                  */
/* Author: Junghan Yoon, KyoungSoo Park                                      */
/* Modified by: (Your Name)                                                  */
/*---------------------------------------------------------------------------*/
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <getopt.h>
#include <errno.h>
#include "common.h"
/*---------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
    char *ip = DEFAULT_LOOPBACK_IP;
    int port = DEFAULT_PORT;
    int interactive = 0; /* Default is non-interactive mode */
    int opt;

    /*---------------------------------------------------------------------------*/
    /* free to declare any variables */
    struct addrinfo hints, *server_info, *p;
    int sockfd = -1;
    int status;
    char port_str[6];
    /*---------------------------------------------------------------------------*/

    /* parse command line options */
    while ((opt = getopt(argc, argv, "i:p:th")) != -1)
    {
        switch (opt)
        {
        case 'i':
            ip = optarg;
            break;
        case 'p':
            port = atoi(optarg);
            if (port <= 1024 || port >= 65536)
            {
                perror("Invalid port number");
                exit(EXIT_FAILURE);
            }
            break;
        case 't':
            interactive = 1;
            break;
        case 'h':
        default:
            printf("Usage: %s [-i server_ip_or_domain (%s)] "
                   "[-p port (%d)] [-t]\n",
                   argv[0],
                   DEFAULT_LOOPBACK_IP,
                   DEFAULT_PORT);
            exit(EXIT_FAILURE);
        }
    }

    /*---------------------------------------------------------------------------*/
    /* edit here */
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    snprintf(port_str, sizeof(port_str), "%d", port);

    if ((status = getaddrinfo(ip, port_str, &hints, &server_info)) != 0)
    {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(EXIT_FAILURE);
    }

    /* Try each address until we successfully connect */
    for (p = server_info; p != NULL; p = p->ai_next)
    {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1)
        {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    freeaddrinfo(server_info);

    if (p == NULL)
    {
        fprintf(stderr, "client: failed to connect\n");
        exit(EXIT_FAILURE);
    }

    if (interactive)
    {
        char line[BUFFER_SIZE];
        printf("Connected to %s:%d\n", ip, port);
        printf("Enter command:\n");

        while (fgets(line, sizeof(line), stdin) != NULL)
        {
            if (strlen(line) <= 1)
                break; // Empty line

            write(sockfd, line, strlen(line));

            char response[BUFFER_SIZE];
            ssize_t bytes_read = read(sockfd, response, sizeof(response) - 1);
            if (bytes_read > 0)
            {
                response[bytes_read] = '\0';
                printf("%s", response);
            }
        }
    }
    else
    {
        /* Silent mode - only process stdin */
        char line[BUFFER_SIZE];
        while (fgets(line, sizeof(line), stdin) != NULL)
        {
            write(sockfd, line, strlen(line));

            char response[BUFFER_SIZE];
            ssize_t bytes_read = read(sockfd, response, sizeof(response) - 1);
            if (bytes_read > 0)
            {
                response[bytes_read] = '\0';
                fprintf(stdout, "%s", response);
                fflush(stdout);
            }
        }
    }

    close(sockfd);
    /*---------------------------------------------------------------------------*/

    return 0;
}