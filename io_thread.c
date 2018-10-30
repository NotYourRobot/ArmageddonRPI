/*
 * File: io_thread.c
 * Usage: no comments
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#include "structs.h"
#include "handler.h"
#include "comm.h"
#include "threads.h"
#include "io_thread.h"
#include "utility.h"
#include "string.h"

extern void threaded_pfind(io_t_msg * msg);
void threaded_dns(io_t_msg * msg);
void resolve_dns(const char *oldip, const char *name);

extern queued_thread *io_thread;

typedef struct __io_thread_data
{
    int align_pad;
 //   MYSQL db_conn;
    int align_pad2;
}
io_thread_data;

void
io_thread_init(queued_thread * thread, thread_data * tldata)
{
    int retval;
    char buf[1024];
    tldata->app_specific = (void *) calloc(1, sizeof(io_thread_data));
    io_thread_data *tdat = tldata->app_specific;
}

void
io_thread_destroy(queued_thread * thread, thread_data * tldata)
{
    io_thread_data *tdat = tldata->app_specific;
    free(tdat);
    tldata->app_specific = 0;
}

void
io_thread_handler(queued_thread * thread, int msg_type, void *data, size_t datalen,
                  thread_data * tldata)
{
    switch (msg_type) {
    case IO_T_MSG_PFIND:
        threaded_pfind((io_t_msg *) data);
        break;
    case IO_T_MSG_DNS_LOOKUP:
        threaded_dns((io_t_msg *) data);
        break;
    }
}

void
io_thread_handle_replies(queued_thread * thread, int msg_type, void *data, size_t datalen,
                         thread_data * tldata)
{
    CHAR_DATA *ch = NULL;
    io_t_msg *msg = alloca(datalen);
    memcpy(msg, data, datalen);

    switch (msg_type) {
    case IO_T_MSG_SEND:
        ch = get_char_pc_account(msg->name, msg->acct);
        if (!ch || !ch->desc) {
            gamelogf("Couldn't find %s to complete send from thread, or recipient unavailable.",
                     msg->name);
            return;
        }
        if (ch->desc->descriptor != msg->desc)
            return;

        send_to_char((char *)msg->data, ch);
        break;
    case IO_T_MSG_PAGED_SEND:
        ch = get_char_pc_account(msg->name, msg->acct);
        if (!ch || !ch->desc) {
            gamelogf("Couldn't find %s to complete send from thread, or recipient unavailable.",
                     msg->name);
            return;
        }
        if (ch->desc->descriptor != msg->desc)
            return;

        page_string(ch->desc, (char *)msg->data, 1);
        break;
    case IO_T_MSG_LOG:
        qgamelogf(QUIET_ZONE, "%s", (char *)msg->data);
        break;
    case IO_T_MSG_SHHLOG:
        shhlog((char *)msg->data);
        break;
    case IO_T_MSG_DNS_LOOKUP:
        resolve_dns(msg->name, (char *)msg->data);
        break;
    }
}

void
io_thread_send_msg(const char *name, const char *acct, int desc, int type, const void *data,
                   size_t datalen)
{
    io_t_msg *outmsg = (io_t_msg *) alloca(sizeof(io_t_msg) + datalen);
    memset(outmsg, 0, sizeof(io_t_msg) + datalen);
    if (name)
        strncpy(outmsg->name, name, sizeof(outmsg->name));
    if (acct)
        strncpy(outmsg->acct, acct, sizeof(outmsg->acct));
    outmsg->desc = desc;
    memcpy(outmsg->data, data, datalen);

    thread_send_message(io_thread->inq, type, outmsg, sizeof(io_t_msg) + datalen);
}

void
io_thread_send_reply(io_t_msg * msg, int type, void *data, size_t datalen)
{
    io_t_msg *outmsg = (io_t_msg *) alloca(sizeof(io_t_msg) + datalen);

    // It is safe for the "header" piece of some messages to be initialized to zero
    // but managing that is up to the person composing new message types
    if (msg) {
        memcpy(outmsg, msg, sizeof(io_t_msg));
    } else {
        memset(outmsg, 0, sizeof(io_t_msg));
    }
    memcpy(outmsg->data, data, datalen);

    thread_send_message(io_thread->outq, type, outmsg, sizeof(io_t_msg) + datalen);
}

void
threaded_dns(io_t_msg * msg)
{   
    int err;

    in_addr_t addr;
    struct hostent *he;         // For the result.  Will == hst_ent on successful resolution
    struct hostent hst_ent;
    char hst_ent_data[1024];
	
    addr = inet_addr((char *)msg->data);
// don't have this for cygwin	
//    gethostbyaddr_r(&addr, sizeof(addr), AF_INET, &hst_ent, hst_ent_data, sizeof(hst_ent_data), &he,
//                    &err);
					
    if (!he) { 
        char errbuf[512];
        snprintf(errbuf, sizeof(errbuf), "DNS Error for %s: %s", msg->data, hstrerror(err));
        io_thread_send_reply( msg, IO_T_MSG_LOG, errbuf, strlen(errbuf) + 1 );
        return;
    } 
    strncpy(msg->name, (char *)msg->data, sizeof(msg->name));
    io_thread_send_reply(msg, IO_T_MSG_DNS_LOOKUP, he->h_name, strlen(he->h_name) + 1);
}

void
resolve_dns(const char *oldip, const char *name)
{
    DESCRIPTOR_DATA *desc;
    for (desc = descriptor_list; desc; desc = desc->next) {
        if (strcmp(desc->host, oldip) == 0) {
            snprintf(desc->hostname, sizeof(desc->hostname), "%s", name);
        }
    }
}


