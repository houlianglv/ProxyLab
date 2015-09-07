#include "cache.h"

/* Return 1 if object length is valid */
int cache_object_length_valid(int length) {
    return length < MAX_OBJECT_SIZE;
}

/* Init cache */
void cache_init(struct Cache *cache) {
    /* Init locks */
    cache->reader = sem_open("/sem.cache.reader", O_CREAT, 0644, 1);
    if(cache->reader == SEM_FAILED){
        unix_error("sem open error");
    }
    cache->writer = sem_open("/sem.cache.writer", O_CREAT, 0644, 1);
    if(cache->writer == SEM_FAILED){
        unix_error("sem open error");
    }
    cache->timer = sem_open("/sem.cache.timer", O_CREAT, 0644, 1);
    if(cache->timer == SEM_FAILED){
        unix_error("sem open error");
    }
    /* Init member vars */
    cache->reader_cnt = 0;
    cache->current_timestamp = 0;
    cache->current_size = 0;
    cache->head = NULL;
}

/* Increase global cache timer synchronizedly */
void cache_tick(struct Cache *cache) {
    P(cache->timer);
    ++cache->current_timestamp;
    V(cache->timer);
}

/* Insert a new pair of (key, object) into cache */
void cache_insert(struct Cache *cache, char *key, int length, void *object) {
    /* Allocate a new Item */
    Item *p = Malloc(sizeof (struct Item));
    /* Copy the key. IMPORTANT! Direct assignment does not work! */
    p->key = Malloc(strlen(key) + 1);
    strcpy(p->key, key);
    p->length = length;
    /* Copy the object */
    p->object = Malloc(length);
    memcpy(p->object, object, length);
    /* Insert Item at the head of linked list */
    p->timestamp = cache->current_timestamp;
    p->next = cache->head;
    cache->head = p;
    /* Increase current cache size */
    cache->current_size += length;
}

/* Remove the least recently used items in cache */
void cache_evict(struct Cache *cache) {
    if (cache->head == NULL)
        app_error("Trying to remove item from an empty cache!");
    Item *prev = NULL, *curr = cache->head;
    Item *candidate_prev = NULL, *candidate_curr = NULL;
    /* Iterate the linked list to find item with lowest timestamp */
    while (curr != NULL) {
        if (candidate_curr == NULL || curr->timestamp < candidate_curr->timestamp) {
            candidate_curr = curr;
            candidate_prev = prev;
        }
        prev = curr;
        curr = curr->next;
    }

    printf("Evicting %s\n", candidate_curr->key);
    /* Decrease cache size */
    cache->current_size -= candidate_curr->length;
    /* Delete node to ensure the integrity of linked list */
    if (candidate_curr == cache->head) {
        cache->head = candidate_curr->next;
    } else {
        candidate_prev->next = candidate_curr->next;
    }
    /* Free memory */
    Free(candidate_curr->key);
    Free(candidate_curr->object);
    Free(candidate_curr);
}

/* Put (key, object) into cache, return 0 if succeed */
int cache_put(struct Cache *cache, char *key, int length, void *object) {
    printf("Cache put %s, length: %d\n", key, length);
    if (!cache_object_length_valid(length))
        return -1;
    
    cache_tick(cache);
    P(cache->writer);
    /* Insert (key, object) into cache */
    cache_insert(cache, key, length, object);
    /* If current cache size exceeds MAX_CACHE_SIZE, evict items until
     * cache size is lower or equal than MAX_CACHE_SIZE
     */
    while (cache->current_size > MAX_CACHE_SIZE) {
        cache_evict(cache);
    }
    V(cache->writer);
    return 0;
}

/* Get (key, object) from cache, return 0 if succeed */
int cache_get(struct Cache *cache, char *key, int* length, void *object) {
    printf("Cache get %s\n", key);
    int result = -1;
    cache_tick(cache);
    P(cache->reader);
    ++cache->reader_cnt;
    if (cache->reader_cnt == 1)
        P(cache->writer); /* if it is the 1st Reader, lock the mutex of writer */
    V(cache->reader);

    /* Iterate each node in linked list */
    Item *p = cache->head;
    while (p != NULL) {
        if (strcmp(p->key, key) == 0) {
            *length = p->length;
            memcpy(object, p->object, p->length);
            p->timestamp = cache->current_timestamp;
            result = 0;
            break;
        }
        p = p->next;
    }

    P(cache->reader);
    --cache->reader_cnt;
    if (cache->reader_cnt == 0)
        V(cache->writer); /* If no reader, free the mutex of writer */
    V(cache->reader);
    return result;
}

/* Prints all objects in cache */
void cache_check(struct Cache *cache) {
    printf("Checking cache...\n");
    /* Iterate each node in linked list */
    Item *p = cache->head;
    while (p != NULL) {
        printf("%s %d %d\n", p->key, p->length, p->timestamp);
        p = p->next;
    }
    printf("Checking cache done!\n");
}