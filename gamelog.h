/*
 * File: gamelog.h */
#ifndef __GAMELOG_H__
#define __GAMELOG_H__

void gamelog(const char *fmt);
void death_log(const char *fmt);
void shhlog(const char *fmt);
void hack_log(const char *fmt);
void errorlog(const char *fmt);
extern void qgamelogf(int quiet_type, const char *fmt, ...) __attribute__ ((format(printf, 2, 3)));
extern void gamelogf(const char *fmt, ...) __attribute__ ((format(printf, 1, 2)));
extern void shhlogf(const char *fmt, ...) __attribute__ ((format(printf, 1, 2)));
extern void hack_logf(const char *fmt, ...) __attribute__ ((format(printf, 1, 2)));
extern void errorlogf(const char *fmt, ...) __attribute__ ((format(printf, 1, 2)));
void shhlog_backtrace();

#endif // __GAMELOG_H__

