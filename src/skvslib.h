/*---------------------------------------------------------------------------*/
/* skvslib.h                                                                 */
/* Author: Junghan Yoon, KyoungSoo Park                                      */
/*---------------------------------------------------------------------------*/
#ifndef _SKVSLIB_H
#define _SKVSLIB_H
/*---------------------------------------------------------------------------*/
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include "hashtable.h"
#include "common.h"
/*---------------------------------------------------------------------------*/
/* response message indices */
enum MSG
{
    MSG_INVALID,
    MSG_CREATE_OK,
    MSG_COLLISION,
    MSG_NOT_FOUND,
    MSG_UPDATE_OK,
    MSG_DELETE_OK,
    MSG_INTERNAL_ERR,
    MSG_COUNT
};
/* command indices */
enum CMD
{
    CMD_INCOMPLETE = -2,
    CMD_INVALID,
    CMD_CREATE,
    CMD_READ,
    CMD_UPDATE,
    CMD_DELETE,
    CMD_COUNT
};
/*---------------------------------------------------------------------------*/
/* SKVS context */
struct skvs_ctx {
    int sock;
    hashtable_t *table;
};
/*---------------------------------------------------------------------------*/
/**
 * initiates SKVS context including a thread-safe global hash table.
 * returns NULL when any internal errors occur.
 * returns the SKVS context pointer on success.
 */
struct skvs_ctx *skvs_init(size_t hash_size, int delay);
/*---------------------------------------------------------------------------*/
/**
 * destroys SKVS context and the hash table.
 * when set dump, dumps the hash table before destroy it.
 * returns -1 when any internal errors occur.
 * returns 0 on success.
 */
int skvs_destroy(struct skvs_ctx *ctx, int dump);
/*---------------------------------------------------------------------------*/
/**
 * returns the complete SKVS commands for the given request on success
 * returns NULL when the request is incomplete.
 * 
 * !Caveat!
 * The return value has no line feed.
 * You should copy the return value to application buffer,
 * and add a line feed at the end.
 */
const char *skvs_serve(struct skvs_ctx *ctx, char *rbuf, size_t rlen);
/*---------------------------------------------------------------------------*/
#endif // _SKVSLIB_H