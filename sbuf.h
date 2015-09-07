/*
 * this is the head file of sbuf(thread-safe buffer)
 * the buffer stores an array with struct type: thread_arg
 * thread_arg is the arg that thread would use in proxy_handler
 *
 */

#ifndef __SBUF_H__
#define __SBUF_H__

#include "csapp.h"


/* thread arg struct */
struct thread_arg{
    int connfd; /* the fd of client */
    struct sockaddr_in clientaddr; /* the socket address of client */
};


/* $begin sbuft */
typedef struct {
    struct thread_arg *buf;          /* Buffer array */
    int n;             /* Maximum number of slots */
    int front;         /* buf[(front+1)%n] is first item */
    int rear;          /* buf[rear%n] is last item */
    sem_t* mutex;       /* Protects accesses to buf */
    sem_t* slots;       /* Counts available slots */
    sem_t* items;       /* Counts available items */
} sbuf_t;
/* $end sbuft */

void sbuf_init(sbuf_t *sp, int n);
void sbuf_deinit(sbuf_t *sp);
void sbuf_insert(sbuf_t *sp, struct thread_arg* item);
struct thread_arg* sbuf_remove(sbuf_t *sp);

#endif /* __SBUF_H__ */