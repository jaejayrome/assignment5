/*---------------------------------------------------------------------------*/
/* server.c                                                                  */
/* Author: Junghan Yoon, KyoungSoo Park                                      */
/* Modified by: (Your Name)                                                  */
/*---------------------------------------------------------------------------*/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <sys/time.h>
#include "common.h"
#include "skvslib.h"
/*---------------------------------------------------------------------------*/
struct thread_args
{
    int listenfd;
    int idx;
    struct skvs_ctx *ctx;

    /*---------------------------------------------------------------------------*/
    /* free to use */

    /*---------------------------------------------------------------------------*/
};
/*---------------------------------------------------------------------------*/
volatile static sig_atomic_t g_shutdown = 0;
/*---------------------------------------------------------------------------*/
void *handle_client(void *arg)
{
    TRACE_PRINT();
    struct thread_args *args = (struct thread_args *)arg;
    struct skvs_ctx *ctx = args->ctx;
    int idx = args->idx;
    int listenfd = args->listenfd;
    /*---------------------------------------------------------------------------*/
    /* free to declare any variables */
    struct sockaddr_storage client_addr;
    socklen_t addr_size = sizeof(client_addr);
    int client_fd;
    char rbuf[BUFFER_SIZE];
    ssize_t n;
    const char *resp;
    /*---------------------------------------------------------------------------*/

    free(args);
    printf("%dth worker ready\n", idx);

    /*---------------------------------------------------------------------------*/
    /* edit here */
    while (!g_shutdown)
    {
        client_fd = accept(listenfd, (struct sockaddr *)&client_addr, &addr_size);
        if (client_fd == -1)
        {
            if (errno == EINTR)
            {
                /* Check if we're shutting down */
                if (g_shutdown)
                    break;
                continue;
            }
            perror("accept");
            break;
        }

        /* Handle client requests */
        while (!g_shutdown && (n = read(client_fd, rbuf, BUFFER_SIZE)) > 0)
        {
            /* Null terminate the received data */
            rbuf[n] = '\0';

            /* Process the request using skvs_serve */
            resp = skvs_serve(ctx, rbuf, n);
            if (resp)
            {
                write(client_fd, resp, strlen(resp));
                write(client_fd, "\n", 1);
            }
        }
        printf("Connection closed by client\n");
        close(client_fd);
    }
    /*---------------------------------------------------------------------------*/

    return NULL;
}
/*---------------------------------------------------------------------------*/
/* Signal handler for SIGINT */
void handle_sigint(int sig)
{
    TRACE_PRINT();
    printf("\nReceived SIGINT, initiating shutdown...\n");
    g_shutdown = 1;
}
/*---------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
    size_t hash_size = DEFAULT_HASH_SIZE;
    char *ip = DEFAULT_ANY_IP;
    int port = DEFAULT_PORT, opt;
    int num_threads = NUM_THREADS;
    int delay = RWLOCK_DELAY;
    /*---------------------------------------------------------------------------*/
    /* free to declare any variables */
    int listenfd;
    struct sockaddr_in server_addr;
    pthread_t *workers;
    struct skvs_ctx *ctx;
    int i, yes = 1;
    /*---------------------------------------------------------------------------*/

    /* parse command line options */
    while ((opt = getopt(argc, argv, "p:t:s:d:h")) != -1)
    {
        switch (opt)
        {
        case 'p':
            port = atoi(optarg);
            break;
        case 't':
            num_threads = atoi(optarg);
            break;
        case 's':
            hash_size = atoi(optarg);
            if (hash_size <= 0)
            {
                perror("Invalid hash size");
                exit(EXIT_FAILURE);
            }
            break;
        case 'd':
            delay = atoi(optarg);
            break;
        case 'h':
        default:
            printf("Usage: %s [-p port (%d)] "
                   "[-t num_threads (%d)] "
                   "[-d rwlock_delay (%d)] "
                   "[-s hash_size (%d)]\n",
                   argv[0],
                   DEFAULT_PORT,
                   NUM_THREADS,
                   RWLOCK_DELAY,
                   DEFAULT_HASH_SIZE);
            exit(EXIT_FAILURE);
        }
    }

    /*---------------------------------------------------------------------------*/
    /* edit here */
    /* edit here */
    /* Initialize SKVS context */
    ctx = skvs_init(hash_size, delay);
    if (!ctx)
    {
        perror("skvs_init failed");
        exit(EXIT_FAILURE);
    }

    /* Block SIGINT before creating threads */
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1)
    {
        perror("sigprocmask");
        skvs_destroy(ctx, 1);
        exit(EXIT_FAILURE);
    }

    /* Create socket */
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0)
    {
        perror("socket creation failed");
        skvs_destroy(ctx, 1);
        exit(EXIT_FAILURE);
    }

    /* Set socket options */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
    {
        perror("setsockopt");
        skvs_destroy(ctx, 1);
        exit(EXIT_FAILURE);
    }

    /* Configure server address */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);

    /* Bind */
    if (bind(listenfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind failed");
        skvs_destroy(ctx, 1);
        exit(EXIT_FAILURE);
    }

    /* Listen */
    if (listen(listenfd, NUM_BACKLOG) < 0)
    {
        perror("listen failed");
        skvs_destroy(ctx, 1);
        exit(EXIT_FAILURE);
    } else {
        printf("Server listening on %s:%d\n", ip, port);
    }

    /* Create worker threads */
    workers = malloc(sizeof(pthread_t) * num_threads);
    if (!workers)
    {
        perror("malloc failed");
        skvs_destroy(ctx, 1);
        exit(EXIT_FAILURE);
    }

    /* Start worker threads */
    for (i = 0; i < num_threads; i++)
    {
        struct thread_args *args = malloc(sizeof(struct thread_args));
        if (!args)
        {
            perror("malloc failed");
            free(workers);
            skvs_destroy(ctx, 1);
            exit(EXIT_FAILURE);
        }
        args->listenfd = listenfd;
        args->idx = i;
        args->ctx = ctx;

        if (pthread_create(&workers[i], NULL, handle_client, args) != 0)
        {
            perror("pthread_create failed");
            free(args);
            free(workers);
            skvs_destroy(ctx, 1);
            exit(EXIT_FAILURE);
        }
    }

    /* Unblock SIGINT after creating threads */
    if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1)
    {
        perror("sigprocmask");
        skvs_destroy(ctx, 1);
        exit(EXIT_FAILURE);
    }

    /* Wait for threads to finish */
    for (i = 0; i < num_threads; i++)
    {
        pthread_join(workers[i], NULL);
    }

    /* Clean up */
    if (g_shutdown)
    {
        hash_dump(ctx->table);
    }
    close(listenfd);
    free(workers);
    skvs_destroy(ctx, 1);
    /*---------------------------------------------------------------------------*/

    return 0;
}