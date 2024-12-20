/*---------------------------------------------------------------------------*/
/* rwlock.c                                                                  */
/* Author: Junghan Yoon, KyoungSoo Park                                      */
/* Modified by: (Your Name)                                                  */
/*---------------------------------------------------------------------------*/
#include "rwlock.h"
/*---------------------------------------------------------------------------*/
int rwlock_init(rwlock_t *rw, int delay)
{
    TRACE_PRINT();
    int ret, destroy_ret;
    rw->read_count = 0;
    rw->write_count = 0;
    rw->writer_ring_head = 0;
    rw->writer_ring_tail = 0;
    rw->delay = delay;
    if (rw->writer_ring)
    {
        free(rw->writer_ring);
    }
    rw->writer_ring = calloc(WRITER_RING_SIZE, sizeof(pthread_t));
    if (!rw->writer_ring)
    {
        return -1;
    }

    ret = pthread_mutex_init(&rw->lock, NULL);
    if (ret != 0)
    {
        errno = ret;
        return -1;
    }

    ret = pthread_cond_init(&rw->readers, NULL);
    if (ret != 0)
    {
        /* mutex destroy failed */
        destroy_ret = pthread_mutex_destroy(&rw->lock);
        if (destroy_ret != 0)
        {
            errno = destroy_ret;
        }
        else
        {
            /* original error from cond init */
            errno = ret;
        }
        return -1;
    }

    ret = pthread_cond_init(&rw->writers, NULL);
    if (ret != 0)
    {
        /* condition variable destroy failed */
        destroy_ret = pthread_cond_destroy(&rw->readers);
        if (destroy_ret != 0)
        {
            errno = destroy_ret;
        }
        else
        {
            /* mutex destroy failed */
            destroy_ret = pthread_mutex_destroy(&rw->lock);
            if (destroy_ret != 0)
            {
                errno = destroy_ret;
            }
            else
            {
                /* original error from cond init */
                errno = ret;
            }
        }
        return -1;
    }

    return 0;
}
/*---------------------------------------------------------------------------*/
int rwlock_read_lock(rwlock_t *rw)
{
    TRACE_PRINT();
    /*---------------------------------------------------------------------------*/
    /* edit here */
    int ret;

    ret = pthread_mutex_lock(&rw->lock);
    if (ret != 0)
    {
        return -1;
    }

    while (rw->write_count > 0 ||
           (rw->writer_ring_head != rw->writer_ring_tail))
    {
        ret = pthread_cond_wait(&rw->readers, &rw->lock);
        if (ret != 0)
        {
            pthread_mutex_unlock(&rw->lock);
            return -1;
        }
    }

    rw->read_count++;

    ret = pthread_mutex_unlock(&rw->lock);
    if (ret != 0)
    {
        return -1;
    }
    /*---------------------------------------------------------------------------*/
    return 0;
}
/*---------------------------------------------------------------------------*/
int rwlock_read_unlock(rwlock_t *rw)
{
    sleep(rw->delay);
    TRACE_PRINT();
    /*---------------------------------------------------------------------------*/
    /* edit here */
    int ret;

    ret = pthread_mutex_lock(&rw->lock);
    if (ret != 0)
    {
        return -1;
    }

    rw->read_count--;

    // If this is the last reader and there are waiting writers
    if (rw->read_count == 0 &&
        rw->writer_ring_head != rw->writer_ring_tail)
    {
        ret = pthread_cond_signal(&rw->writers);
        if (ret != 0)
        {
            pthread_mutex_unlock(&rw->lock);
            return -1;
        }
    }

    ret = pthread_mutex_unlock(&rw->lock);
    if (ret != 0)
    {
        return -1;
    }
    /*---------------------------------------------------------------------------*/
    return 0;
}
/*---------------------------------------------------------------------------*/
int rwlock_write_lock(rwlock_t *rw)
{
    TRACE_PRINT();
    /*---------------------------------------------------------------------------*/
    /* edit here */
    int ret;

    ret = pthread_mutex_lock(&rw->lock);
    if (ret != 0)
    {
        return -1;
    }

    rw->writer_ring[rw->writer_ring_tail] = pthread_self();
    rw->writer_ring_tail = (rw->writer_ring_tail + 1) % WRITER_RING_SIZE;

    while (rw->read_count > 0 || rw->write_count > 0 ||
           pthread_self() != rw->writer_ring[rw->writer_ring_head])
    {
        ret = pthread_cond_wait(&rw->writers, &rw->lock);
        if (ret != 0)
        {
            pthread_mutex_unlock(&rw->lock);
            return -1;
        }
    }

    rw->write_count++;
    rw->writer_ring_head = (rw->writer_ring_head + 1) % WRITER_RING_SIZE;

    ret = pthread_mutex_unlock(&rw->lock);
    if (ret != 0)
    {
        return -1;
    }
    /*---------------------------------------------------------------------------*/
    return 0;
}
/*---------------------------------------------------------------------------*/
int rwlock_write_unlock(rwlock_t *rw)
{
    sleep(rw->delay);
    TRACE_PRINT();
    /*---------------------------------------------------------------------------*/
    /* edit here */
    int ret;

    ret = pthread_mutex_lock(&rw->lock);
    if (ret != 0)
    {
        return -1;
    }

    rw->write_count--;

    if (rw->read_count > 0)
    {
        ret = pthread_cond_broadcast(&rw->readers);
        if (ret != 0)
        {
            pthread_mutex_unlock(&rw->lock);
            return -1;
        }
    }
    else
    {
        ret = pthread_cond_signal(&rw->writers);
        if (ret != 0)
        {
            pthread_mutex_unlock(&rw->lock);
            return -1;
        }
    }

    ret = pthread_mutex_unlock(&rw->lock);
    if (ret != 0)
    {
        return -1;
    }

    /*---------------------------------------------------------------------------*/
    return 0;
}
/*---------------------------------------------------------------------------*/
int rwlock_destroy(rwlock_t *rw)
{
    TRACE_PRINT();
    int ret;

    /* destroy the mutex */
    ret = pthread_mutex_destroy(&rw->lock);
    if (ret != 0)
    {
        errno = ret;
        return -1;
    }

    /* destroy the readers condition variable */
    ret = pthread_cond_destroy(&rw->readers);
    if (ret != 0)
    {
        errno = ret;
        return -1;
    }

    /* destroy the writers condition variable */
    ret = pthread_cond_destroy(&rw->writers);
    if (ret != 0)
    {
        errno = ret;
        return -1;
    }

    if (rw->writer_ring)
    {
        free(rw->writer_ring);
        rw->writer_ring = NULL;
    }

    return 0;
}