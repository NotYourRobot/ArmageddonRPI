/*
 * File: SIGNALS.C
 * Usage: Trapping signals.
 *
 * Copyright (C) 1993, 1994 by Dan Brumleve, Brian Takle, and the creators
 * of DikuMUD.  May Vivadu bring them good health and many children!
 *
 *   /\    ____                    PART OF          __     ____   |\   |
 *  /__\   |___|   /\  /\    /\    __  ___  +--\   |  \   /    \  | \  |
 * /    \  |   \  /  \/  \  /--\  /    [_   |   \  |   |  |    |  |  \ |
 * \    /  |   /  \      /  \  /  \_]  [__  |___/  |__/   \____/  |   \|
 * ---------------------------------------------------------------------
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "structs.h"
#include "utils.h"
#include "utility.h"
#include "comm.h"

/* setting this to 1 will interrupt any currently running asl program */
void checkpointing(int);
void shutdown_request(int);
void hupsig(int);
void sigignore(int ignore);

void
signal_setup_JIM(void)
{
    /* just to be on the safe side: */
    signal(SIGHUP, &hupsig);
    signal(SIGINT, &hupsig);
    signal(SIGALRM, &hupsig);


    /* gamelog a signal, but ignore these for the most part */
    signal(SIGTERM, &sigignore);
    signal(SIGQUIT, &sigignore);
    signal(SIGILL, &sigignore);
    signal(SIGABRT, &sigignore);
    signal(SIGFPE, &sigignore);
    signal(SIGKILL, &sigignore);
    signal(SIGSEGV, &sigignore);
    signal(SIGPIPE, &sigignore);

    signal(SIGALRM, &sigignore);

    signal(SIGTERM, &sigignore);
    signal(SIGUSR1, &sigignore);
    signal(SIGUSR2, &sigignore);
    signal(SIGCHLD, &sigignore);
    signal(SIGCONT, &sigignore);
    signal(SIGSTOP, &sigignore);
    signal(SIGTSTP, &sigignore);
    signal(SIGTTIN, &sigignore);


}

void
signal_setup(void)
{
    struct itimerval itime;
    struct timeval interval;

    signal(SIGUSR2, &shutdown_request);

    /* just to be on the safe side: */

    signal(SIGHUP, &hupsig);
    /* signal(SIGPIPE, SIG_IGN); 
     * redundant, handled by dc_select() in comm.c */

    signal(SIGINT, &hupsig);
    signal(SIGTERM, &hupsig);

    /* set up the deadlock-protection */

    interval.tv_sec = 900;      /* 15 minutes */
    interval.tv_usec = 0;
    itime.it_interval = interval;
    itime.it_value = interval;
    /*    setitimer(ITIMER_VIRTUAL, &itime, 0); */
    /*    signal(SIGVTALRM, &checkpointing); 
     * !needed, causes 5 hr crashes  -Morg */

}




void
checkpointing(int ignore)
{
    if (!tics) {
        gamelog("CHECKPOINT shutdown: tics not updated");
        abort();
    } else
        tics = 0;
}

void
shutdown_request(int ignore)
{
    gamelog("Received USR2 - shutdown request");
    shutdown_game = 1;
}


/* kick out players etc */
void
hupsig(int ignore)
{
    gamelog("Received SIGHUP, SIGINT, or SIGTERM. Shutting down.");
    exit(0);
}

void
sigignore(int signum)
{
    char test[400];

    shhlog("SIGNAL:received an ignoreable signal");
    sprintf(test, "SIGNAL:signal = %d", signum);
    shhlog(test);
}

