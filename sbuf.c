/* $begin sbufc */
#include "csapp.h"
#include "sbuf.h"

/* Create an empty, bounded, shared FIFO buffer with n slots */
/* $begin sbuf_init */
void sbuf_init(sbuf_t *sp, int n)
{
    sp->buf = Calloc(n, sizeof(struct thread_arg));
    sp->n = n;                       /* Buffer holds max of n items */
    sp->front = sp->rear = 0;        /* Empty buffer iff front == rear */

    sem_t *sem;   /* sem_init is deperacted, use sem_open instead */
    /* sem_open create named semaphore, the implementation is based on file */
    if((sem = sem_open("/sem.mutex", O_CREAT, 0644, 1)) == SEM_FAILED){
        unix_error("Sem_open error");
    }else{
        sp->mutex = sem;
    }

    if((sem = sem_open("/sem.slots", O_CREAT, 0644, n)) == SEM_FAILED){
        unix_error("Sem_open error");
    }else{
        sp->slots = sem;
    }

    if((sem = sem_open("/sem.items", O_CREAT, 0644, 0)) == SEM_FAILED){
        unix_error("Sem_open error");
    }else{
        sp->items = sem;
    }
}
/* $end sbuf_init */

/* Clean up buffer sp */
/* $begin sbuf_deinit */
void sbuf_deinit(sbuf_t *sp)
{
    Free(sp->buf);
}
/* $end sbuf_deinit */

/* Insert item onto the rear of shared buffer sp */
/* $begin sbuf_insert */
void sbuf_insert(sbuf_t *sp, struct thread_arg *item)
{
    P(sp->slots);                          /* Wait for available slot */
    P(sp->mutex);                          /* Lock the buffer */
    sp->buf[(++sp->rear)%(sp->n)] = *item;   /* Insert the item */
    V(sp->mutex);                          /* Unlock the buffer */
    V(sp->items);                          /* Announce available item */
}
/* $end sbuf_insert */

/* Remove and return the first item from buffer sp */
/* $begin sbuf_remove */
struct thread_arg *sbuf_remove(sbuf_t *sp)
{
    struct thread_arg *item;
    P(sp->items);                          /* Wait for available item */
    P(sp->mutex);                          /* Lock the buffer */
    item = &sp->buf[(++sp->front)%(sp->n)];  /* Remove the item */
    V(sp->mutex);                          /* Unlock the buffer */
    V(sp->slots);                          /* Announce available slot */
    return item;
}
/* $end sbuf_remove */
/* $end sbufc */
