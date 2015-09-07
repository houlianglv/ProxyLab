#ifndef __proxylab__cache__
#define __proxylab__cache__

#include "csapp.h"
/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/*
 * Linked list node
 */
typedef struct Item {
    char *key;
    void *object;
    int timestamp; /* For LRU approximating */
    int length;
    struct Item *next;
} Item;

/*
 * Cache is composed of a linked list, a read-write lock.
 */
typedef struct Cache {
    sem_t *reader; /* Reader lock syncs ops on 'reader_cnt' */
    sem_t *writer; /* Writer lock syncs ops on whole cache */
    sem_t *timer;  /* Timer lock syncs ops on 'current_timestamp' */
    int reader_cnt;
    int current_timestamp;
    int current_size;
    struct Item *head; /* Linked list head */
} Cache;

/* Return 1 if object length is valid */
int cache_object_length_valid(int length);
/* Init cache */
void cache_init(struct Cache *cache);
/* Put (key, object) into cache, return 0 if succeed */
int cache_put(struct Cache *cache, char *key, int length, void *object);
/* Get (key, object) from cache, return 0 if succeed */
int cache_get(struct Cache *cache, char *key, int* length, void *object);
/* Prints all objects in cache */
void cache_check(struct Cache *cache);
#endif /* defined(__proxylab__cache__) */
