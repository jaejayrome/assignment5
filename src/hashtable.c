/*---------------------------------------------------------------------------*/
/* hashtable.c                                                               */
/* Author: Junghan Yoon, KyoungSoo Park                                      */
/* Modified by: Jerome Goh Zhi Sheng                                               */
/*---------------------------------------------------------------------------*/
#include "hashtable.h"
/*---------------------------------------------------------------------------*/
int hash(const char *key, size_t hash_size)
{
    TRACE_PRINT();
    unsigned int hash = 0;
    while (*key)
    {
        hash = (hash << 5) + *key++;
    }

    return hash % hash_size;
}
/*---------------------------------------------------------------------------*/
hashtable_t *hash_init(size_t hash_size, int delay)
{
    TRACE_PRINT();
    int i, j, ret;
    hashtable_t *table = malloc(sizeof(hashtable_t));

    if (table == NULL)
    {
        DEBUG_PRINT("Failed to allocate memory for hash table");
        return NULL;
    }

    table->hash_size = hash_size;
    table->total_entries = 0;

    table->buckets = malloc(hash_size * sizeof(node_t *));
    if (table->buckets == NULL)
    {
        DEBUG_PRINT("Failed to allocate memory for hash table buckets");
        free(table);
        return NULL;
    }

    table->locks = malloc(hash_size * sizeof(rwlock_t));
    if (table->locks == NULL)
    {
        DEBUG_PRINT("Failed to allocate memory for hash table locks");
        free(table->buckets);
        free(table);
        return NULL;
    }

    table->bucket_sizes = malloc(hash_size * sizeof(*table->bucket_sizes));
    if (table->bucket_sizes == NULL)
    {
        DEBUG_PRINT("Failed to allocate memory for hash table bucket sizes");
        free(table->buckets);
        free(table->locks);
        free(table);
        return NULL;
    }

    for (i = 0; i < hash_size; i++)
    {
        table->buckets[i] = NULL;
        table->bucket_sizes[i] = 0;
        ret = rwlock_init(&table->locks[i], delay);
        if (ret != 0)
        {
            DEBUG_PRINT("Failed to initialize read-write lock");
            for (j = 0; j < i; j++)
            {
                rwlock_destroy(&table->locks[j]);
            }
            free(table->buckets);
            free(table->locks);
            free(table->bucket_sizes);
            free(table);
            return NULL;
        }
    }

    return table;
}
/*---------------------------------------------------------------------------*/
int hash_destroy(hashtable_t *table)
{
    TRACE_PRINT();
    node_t *node, *tmp;
    int i;

    for (i = 0; i < table->hash_size; i++)
    {
        node = table->buckets[i];
        while (node)
        {
            tmp = node;
            node = node->next;
            free(tmp->key);
            free(tmp->value);
            free(tmp);
        }
        if (rwlock_destroy(&table->locks[i]) != 0)
        {
            DEBUG_PRINT("Failed to destroy read-write lock");
            return -1;
        }
    }

    free(table->buckets);
    free(table->locks);
    free(table->bucket_sizes);
    free(table);

    return 0;
}
/*---------------------------------------------------------------------------*/
int hash_insert(hashtable_t *table, const char *key, const char *value)
{
    TRACE_PRINT();
    node_t *node;
    rwlock_t *lock;
    unsigned int index = hash(key, table->hash_size);

    /*---------------------------------------------------------------------------*/
    /* edit here */
    lock = &table->locks[index];
    if (rwlock_write_lock(lock) != 0)
    {
        return -1; // Lock acquisition failed
    }

    /* Check if key already exists */
    node = table->buckets[index];
    while (node)
    {
        if (strcmp(node->key, key) == 0)
        {
            rwlock_write_unlock(lock);
            return 0; // Collision
        }
        node = node->next;
    }

    /* Create new node */
    node = malloc(sizeof(node_t));
    if (!node)
    {
        rwlock_write_unlock(lock);
        return -1;
    }

    /* Duplicate key and value */
    node->key = strdup(key);
    node->value = strdup(value);
    if (!node->key || !node->value)
    {
        free(node->key); // One might be NULL
        free(node->value);
        free(node);
        rwlock_write_unlock(lock);
        return -1;
    }

    node->key_size = strlen(key);
    node->value_size = strlen(value);

    /* Insert at head of bucket */
    node->next = table->buckets[index];
    table->buckets[index] = node;
    table->bucket_sizes[index]++;
    table->total_entries++;

    rwlock_write_unlock(lock);

    /*---------------------------------------------------------------------------*/

    /* inserted */
    return 1;
}
/*---------------------------------------------------------------------------*/
int hash_search(hashtable_t *table, const char *key, const char **value)
{
    TRACE_PRINT();
    node_t *node;
    rwlock_t *lock;
    unsigned int index = hash(key, table->hash_size);

    /*---------------------------------------------------------------------------*/
    /* edit here */
    lock = &table->locks[index];
    if (rwlock_read_lock(lock) != 0)
    {
        return -1; // Lock acquisition failed
    }

    /* Search for key */
    node = table->buckets[index];
    while (node)
    {
        if (strcmp(node->key, key) == 0)
        {
            *value = node->value;
            rwlock_read_unlock(lock);
            return 1; // Found
        }
        node = node->next;
    }

    rwlock_read_unlock(lock);

    /*---------------------------------------------------------------------------*/

    /* key not found */
    return 0;
}
/*---------------------------------------------------------------------------*/
int hash_update(hashtable_t *table, const char *key, const char *value)
{
    TRACE_PRINT();
    node_t *node;
    rwlock_t *lock;
    char *new_value;
    unsigned int index = hash(key, table->hash_size);

    /*---------------------------------------------------------------------------*/
    /* edit here */
    lock = &table->locks[index];
    if (rwlock_write_lock(lock) != 0)
    {
        return -1; // Lock acquisition failed
    }

    /* Search for key */
    node = table->buckets[index];
    while (node)
    {
        if (strcmp(node->key, key) == 0)
        {
            /* Duplicate new value */
            new_value = strdup(value);
            if (!new_value)
            {
                rwlock_write_unlock(lock);
                return -1;
            }

            /* Free old value and update */
            free(node->value);
            node->value = new_value;
            node->value_size = strlen(value);

            rwlock_write_unlock(lock);
            return 1; // Updated
        }
        node = node->next;
    }

    rwlock_write_unlock(lock);

    /*---------------------------------------------------------------------------*/

    /* key not found */
    return 0;
}
/*---------------------------------------------------------------------------*/
int hash_delete(hashtable_t *table, const char *key)
{
    TRACE_PRINT();
    node_t *node, *prev;
    rwlock_t *lock;
    unsigned int index = hash(key, table->hash_size);

    /*---------------------------------------------------------------------------*/
    /* edit here */
    lock = &table->locks[index];
    if (rwlock_write_lock(lock) != 0)
    {
        return -1; // Lock acquisition failed
    }

    /* Search for key */
    prev = NULL;
    node = table->buckets[index];
    while (node)
    {
        if (strcmp(node->key, key) == 0)
        {
            /* Update bucket list */
            if (prev)
                prev->next = node->next;
            else
                table->buckets[index] = node->next;

            /* Free node */
            free(node->key);
            free(node->value);
            free(node);

            table->bucket_sizes[index]--;
            table->total_entries--;

            rwlock_write_unlock(lock);
            return 1; // Deleted
        }
        prev = node;
        node = node->next;
    }

    rwlock_write_unlock(lock);

    /*---------------------------------------------------------------------------*/

    /* key not found */
    return 0;
}
/*---------------------------------------------------------------------------*/
/* function to dump the contents of the hash table, including locks status */
void hash_dump(hashtable_t *table)
{
    TRACE_PRINT();
    node_t *node;
    int i;

    printf("[Hash Table Dump]");
    printf("Total Entries: %ld\n", table->total_entries);

    for (i = 0; i < table->hash_size; i++)
    {
        if (!table->bucket_sizes[i])
        {
            continue;
        }
        printf("Bucket %d: %ld entries\n", i, table->bucket_sizes[i]);
        printf("  Lock State -> Read Count: %d, Write Count: %d\n",
               table->locks[i].read_count, table->locks[i].write_count);
        node = table->buckets[i];
        while (node)
        {
            printf("    Key:   %s\n"
                   "    Value: %s\n",
                   node->key, node->value);
            node = node->next;
        }
    }
    printf("End of Dump\n");
}