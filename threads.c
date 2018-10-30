/*
 * File: threads.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
// #include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <ctype.h>

#include "threads.h"

int queue_pop_message(message_queue * queue, queued_message ** msg);
int queue_push_message(message_queue * queue, queued_message * msg);
void terminate_queued_thread(queued_thread * thread);
void usleep(unsigned long usec);

void *
queued_thread_main(void *arg)
{
    queued_message *msg;
    queued_thread *thread = arg;

    transition_handler_func *initf = (transition_handler_func *) thread->init_func;
    transition_handler_func *destroyf = (transition_handler_func *) thread->destroy_func;

    initf(thread, &thread->tldata);

    message_handler_func *func = (message_handler_func *) thread->mh_func;
    for (;;) {
        int internal_type = 0;
        pthread_t internal_thread_id = 0;
        int got_msg = queue_pop_message(thread->inq, &msg);

        if (!got_msg) {
            usleep(50);
            continue;
        }

        internal_type = msg->int_type;
        internal_thread_id = msg->int_thread_id;
        switch (internal_type) {
        case MSG_APP_DATA:
            func(thread, msg->app_type, msg->app_data, msg->app_data_len, &thread->tldata);
            break;
        case MSG_THREAD_EXIT:
            if (pthread_equal(internal_thread_id, pthread_self())) {    // It's for me
                thread_release_message(thread, msg);
                goto out;          // Exit from for(;;)
            } else {
                queue_push_message(thread->inq, msg);   // push it back on there for another thread to find
                continue;       // Do not do the "release_message()" coming up
            }
        }

        thread_release_message(thread, msg);
    }

out:
    destroyf(thread, &thread->tldata);
    return 0;
}

queued_thread *
create_queued_thread(transition_handler_func * init_handler,
                     transition_handler_func * destroy_handler, message_handler_func * msg_handler,
                     message_queue * inq, message_queue * outq)
{
    queued_thread *new_thread = (queued_thread *) calloc(sizeof(queued_thread), 1);
    new_thread->inq = inq;
    new_thread->outq = outq;

    new_thread->mh_func = msg_handler;
    new_thread->init_func = init_handler;
    new_thread->destroy_func = destroy_handler;

    pthread_create(&new_thread->handle, NULL, queued_thread_main, new_thread);
    return new_thread;
}

void
destroy_queued_thread(queued_thread * thread)
{
    terminate_queued_thread(thread);
    free(thread);
}

int
queue_pop_message(message_queue * queue, queued_message ** msg)
{
    int got_msg = 0;
    pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &queue->qmutex);
    pthread_mutex_lock(&queue->qmutex);

    if (queue->last) {
        got_msg = 1;
        *msg = queue->last;
        queue->last = (*msg)->prev;
        if (queue->last) {
            queue->last->next = NULL;
        }

        if (queue->first == *msg) {
            queue->first = NULL;
        }
    }

    pthread_cleanup_pop(1);     // Release mutex

    return got_msg;
}

void
queue_init(message_queue *queue)
{
    memset(queue, 0, sizeof(*queue));
    pthread_mutex_init(&queue->qmutex, NULL);
}

int
queue_push_message(message_queue * queue, queued_message * msg)
{
    msg->prev = NULL;

    pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &queue->qmutex);
    pthread_mutex_lock(&queue->qmutex);

    msg->next = queue->first;
    if (queue->first)
        queue->first->prev = msg;
    queue->first = msg;

    if (!queue->last)
        queue->last = msg;

    pthread_cleanup_pop(1);     // Release mutex
    return 1;
}

int
queue_empty(message_queue * queue)
{
    int empty = 0;
    pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &queue->qmutex);
    pthread_mutex_lock(&queue->qmutex);
    if (!queue->first && !queue->last)
        empty = 1;
    pthread_cleanup_pop(1);     // Release mutex
    return empty;
}

void
thread_send_message(message_queue * queue, int type, void *data, int datalen)
{
    queued_message *newmess = (queued_message *) calloc(sizeof(queued_message) + datalen, 1);
    newmess->int_type = MSG_APP_DATA;
    newmess->app_type = type;
    newmess->app_data = (char *) newmess + sizeof(queued_message);
    newmess->app_data_len = datalen;
    memcpy(newmess->app_data, data, datalen);
    queue_push_message(queue, newmess);
}

int
thread_recv_message(queued_thread * thread, message_queue * queue, message_handler_func * handler)
{
    queued_message *msg;
    if (!queue_pop_message(queue, &msg))
        return 0;

    switch (msg->int_type) {
    case MSG_RELEASE_MSG:
        free(msg->app_data);
        break;
    case MSG_APP_DATA:
        if (handler)
            handler(thread, msg->app_type, msg->app_data, msg->app_data_len, &thread->tldata);
        break;
    }

    free(msg);
    return 1;
}

void
thread_release_message(queued_thread * thread, queued_message * message)
{
    free(message);
}

void
terminate_queued_thread(queued_thread * thread)
{
    unsigned long retval;

    queued_message *newmess = (queued_message *) calloc(sizeof(queued_message), 1);
    newmess->int_type = MSG_THREAD_EXIT;
    newmess->int_thread_id = thread->handle;
    queue_push_message(thread->inq, newmess);

    pthread_join(thread->handle, (void *) &retval);
    while (thread_recv_message(thread, thread->outq, NULL)) ;
}

#ifdef UNIT_TEST
#include <stdarg.h>

#define IO_THREAD_POOL_SIZE  50
queued_thread *io_thread[IO_THREAD_POOL_SIZE];

#include "io_thread.h"

void
io_thread_handler(queued_thread * thread, int msg_type, void *data, size_t datalen)
{
}

void
shhlogf(const char *fmt, ...)
{
    char buf[1024];
    va_list argp;

    va_start(argp, fmt);
    vsnprintf(buf, sizeof(buf), fmt, argp);
    va_end(argp);

    printf("/* %s */\n", buf);
}


int
main(int argc, const char *const argv[])
{
    int i, index = 0, loops = random() % 40;
    srandom(time(NULL));
    printf("Starting\n");

    message_queue iot_inq, iot_outq;
    memset(&iot_inq, 0, sizeof(iot_inq));
    memset(&iot_outq, 0, sizeof(iot_outq));

    for (i = 0; i < IO_THREAD_POOL_SIZE; i++) {
        io_thread[i] = create_queued_thread(io_thread_handler, &iot_inq, &iot_outq);
    }

    for (i = 0; i < IO_THREAD_POOL_SIZE; i++) {
        shhlogf("joining thread %d", io_thread[i]->handle);
        destroy_queued_thread(io_thread[i]);
    }
/*
    for (;;) {
//    char echoline[128]; echoline[0] = '\0';
//    if (fgets(echoline, sizeof(echoline), stdin) != 0) {
//      thread_send_message( &thread->inq, SYSTEM_CMD, echoline, strlen(echoline) + 1 );
//    }
        for (i = 0; i < loops; i++) {
            char *tmp = malloc(random() % 100);
            free(tmp);
        }
        printf("%d\r", index);

        io_thread_send_msg("Xygax", "Xygax", 1, IO_T_MSG_PFIND, "Nessalin", 9);

        thread_recv_message(io_thread, &io_thread->outq, &io_thread_handle_replies);

//    if (feof(stdin)) {
        if (index++ > loops) {
            while (!queue_empty(&io_thread->inq)) {
                thread_recv_message(io_thread, &io_thread->outq, &io_thread_handle_replies);
            }

            terminate_queued_thread(io_thread);
            while (thread_recv_message(io_thread, &io_thread->outq, &io_thread_handle_replies));
            free(io_thread);

            return (0);
        }
//    usleep( 5 );
    }
*/
    printf("\n");

    return 0;
}

#endif // UNIT_TEST

