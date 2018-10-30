/*
 * File: io_thread.h */
#include "threads.h"

enum IO_T_MSG_TYPES
{
    IO_T_MSG_PFIND = 1,
    IO_T_MSG_SEND,
    IO_T_MSG_PAGED_SEND,
    IO_T_MSG_LOG,
    IO_T_MSG_SHHLOG,
    IO_T_MSG_DNS_LOOKUP,
    IO_T_MSG_SQL_QUERY_SEND,
};


typedef struct __io_t_msg
{
    char name[64];
    char acct[64];
    int desc;                   // fileno
    unsigned char data[1];
}
io_t_msg;

void io_thread_init(queued_thread * thread, thread_data * tldata);
void io_thread_destroy(queued_thread * thread, thread_data * tldata);
void io_thread_handler(queued_thread * thread, int msg_type, void *data, size_t datalen,
                       thread_data * tldata);
void io_thread_handle_replies(queued_thread * thread, int msg_type, void *data, size_t datalen,
                              thread_data * tldata);
void io_thread_send_msg(const char *name, const char *acct, int desc, int type, const void *data,
                        size_t datalen);
void io_thread_send_reply(io_t_msg * msg, int type, void *data, size_t datalen);

//void io_thread_submit_sql(const char *query);


