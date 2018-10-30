/*
 * File: threads.h */
#ifndef __THREADS_H__
#define __THREADS_H__


#define MSG_APP_DATA    (unsigned char)0x1
#define MSG_THREAD_EXIT (unsigned char)0x2
#define MSG_RELEASE_MSG (unsigned char)0x3

typedef struct __queued_msg_t
{
    unsigned char int_type;     // Internal message type
    pthread_t int_thread_id;    // Internal thread ID; for things like THREAD_EXIT msg
    unsigned app_type;          // Message type assigned by the application
    void *app_data;
    size_t app_data_len;
    struct __queued_msg_t *next, *prev;
}
queued_message;

typedef struct __message_queue_t
{
    pthread_mutex_t qmutex;
    queued_message *first;
    queued_message *last;
}
message_queue;

typedef struct __thread_data_t
{
    void *app_specific;
}
thread_data;

typedef struct __queued_thread_t
{
    pthread_t handle;
    message_queue *inq;         // To thread
    message_queue *outq;        // From thread
    void *init_func;
    void *destroy_func;
    void *mh_func;
    thread_data tldata;         // Passed to mh_func to be used for "thread local storage" essentially
}
queued_thread;

typedef
    void (message_handler_func
          (queued_thread *thread, int msg_type, void *data, size_t datalen, thread_data *tldata));

typedef void (transition_handler_func(queued_thread *thread, thread_data *tldata));

queued_thread *create_queued_thread(transition_handler_func *init_handler,
                                    transition_handler_func *destroy_handler,
                                    message_handler_func *msg_handler, message_queue *inq,
                                    message_queue *outq);
void destroy_queued_thread(queued_thread *thread);

void queue_init(message_queue *queue);
void thread_send_message(message_queue *queue, int type, void *data, int datalen);
int thread_recv_message(queued_thread *thread, message_queue *queue,
                        message_handler_func *handler);
void thread_release_message(queued_thread *queue, queued_message *msg);

#endif // __THREADS_H__

