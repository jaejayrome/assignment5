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

    /* Convert port to string */
    snprintf(port_str, sizeof(port_str), "%d", port);

    /* Get address information */
    if ((status = getaddrinfo(ip, port_str, &hints, &server_info)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(EXIT_FAILURE);
    }

    /* Walking the LinkedList */
    for(p = server_info; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) {
            perror("client: socket");
            continue;
        }

        // if client is unable to connect to the server
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        // break only when client is able to connct to the server
        break;
    }

    /* Clean up address info */
    freeaddrinfo(server_info);

    /* Check if connection failed */
    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        exit(EXIT_FAILURE);
    }

    printf("Connected to %s:%d\n", ip, port);

    /* Handle interactive mode */
    if (interactive) {
        printf("Running in interactive mode\n");
    }

    /* Clean up socket */
    close(sockfd);
/*---------------------------------------------------------------------------*/

    return 0;
}