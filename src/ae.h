/* A simple event-driven programming library. Originally I wrote this code
 * for the Jim's event-loop (Jim is a Tcl interpreter) but later translated
 * it in form of a library for easy reuse.
 *
 * Copyright (c) 2006-2012, Salvatore Sanfilippo <antirez at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __AE_H__
#define __AE_H__

#include <time.h>

#define AE_OK 0
#define AE_ERR -1

#define AE_NONE 0       /* No events registered. */
#define AE_READABLE 1   /* Fire when descriptor is readable. */
#define AE_WRITABLE 2   /* Fire when descriptor is writable. */
#define AE_BARRIER 4    /* With WRITABLE, never fire the event if the
                           READABLE event already fired in the same event
                           loop iteration. Useful when you want to persist
                           things to disk before sending replies, and want
                           to do that in a group fashion. */

#define AE_FILE_EVENTS (1<<0)
#define AE_TIME_EVENTS (1<<1)
#define AE_ALL_EVENTS (AE_FILE_EVENTS|AE_TIME_EVENTS)
#define AE_DONT_WAIT (1<<2)
#define AE_CALL_BEFORE_SLEEP (1<<3)
#define AE_CALL_AFTER_SLEEP (1<<4)

#define AE_NOMORE -1
#define AE_DELETED_EVENT_ID -1

/* Macros */
#define AE_NOTUSED(V) ((void) V)

struct aeEventLoop;

/* Types and data structures */

/* 文件事件处理函数原型 */
typedef void aeFileProc(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask);
/* 时间事件处理函数原型 */
typedef int aeTimeProc(struct aeEventLoop *eventLoop, long long id, void *clientData);
/* 事务释放函数 */
typedef void aeEventFinalizerProc(struct aeEventLoop *eventLoop, void *clientData);
/* 在sleep前的处理函数 */
typedef void aeBeforeSleepProc(struct aeEventLoop *eventLoop);

/* File event structure */
/* 文件事务结构体 */
typedef struct aeFileEvent {
    int mask; /* one of AE_(READABLE|WRITABLE|BARRIER) */
    aeFileProc *rfileProc; /* 读事件（AE_READABLE)处理函数 */
    aeFileProc *wfileProc; /* 写事件（AE_WRITABLE)处理函数 */
    void *clientData;      /* 客户端数据 */
} aeFileEvent;

/* Time event structure */
/* 时间事务结构体 */
typedef struct aeTimeEvent {
    long long id; /* time event identifier. 时间事务id */
    /* 何时触发这个时间 */
    long when_sec; /* seconds 秒*/
    long when_ms; /* milliseconds 毫秒*/
    aeTimeProc *timeProc; /* 时间事件处理函数 */
    aeEventFinalizerProc *finalizerProc; /* 事件释放函数 */
    void *clientData;  /* 客户端数据 */
    struct aeTimeEvent *prev; /* 指向上一个时间事务 */
    struct aeTimeEvent *next; /* 指向下一个时间事务 */
    int refcount; /* refcount to prevent timer events from being
  		           * freed in recursive time event calls. 
                   * 当前时间事务的引用计数来避免在循环时间事务调用中被释放掉。*/
} aeTimeEvent;

/* A fired event */  
/* 已经就绪的事务 */
typedef struct aeFiredEvent {
    int fd;   /* 已就绪的事务的文件描述符 */
    int mask; /* 已就绪的事务的操作 */
} aeFiredEvent;

/* State of an event based program */
/* 事务处理程序的状态 */
typedef struct aeEventLoop {
    int maxfd;   /* highest file descriptor currently registered；当前注册的最大的文件描述符 */
    int setsize; /* max number of file descriptors tracked；目前已追踪的文件描述符的最大数量*/
    long long timeEventNextId; /* 时间事务的下一个id */
    time_t lastTime;     /* Used to detect system clock skew；用来检测系统时钟的偏移 */
    aeFileEvent *events; /* Registered events；已注册的文件事务 */
    aeFiredEvent *fired; /* Fired events；已就绪的事务 */
    aeTimeEvent *timeEventHead;  /* 时间事务的头节点（时间事务构成一个双向链表） */
    int stop;  /* 事件处理其的开关 */
    void *apidata; /* This is used for polling API specific data；这个指针是指向轮询api的aeApiState结构数据 */
    aeBeforeSleepProc *beforesleep; /* 处理事务前要执行的函数 */
    aeBeforeSleepProc *aftersleep; /* 处理事务后要执行的函数 */
    int flags;
} aeEventLoop;

/* Prototypes */
aeEventLoop *aeCreateEventLoop(int setsize);
void aeDeleteEventLoop(aeEventLoop *eventLoop);
void aeStop(aeEventLoop *eventLoop);
int aeCreateFileEvent(aeEventLoop *eventLoop, int fd, int mask,
        aeFileProc *proc, void *clientData);
void aeDeleteFileEvent(aeEventLoop *eventLoop, int fd, int mask);
int aeGetFileEvents(aeEventLoop *eventLoop, int fd);
long long aeCreateTimeEvent(aeEventLoop *eventLoop, long long milliseconds,
        aeTimeProc *proc, void *clientData,
        aeEventFinalizerProc *finalizerProc);
int aeDeleteTimeEvent(aeEventLoop *eventLoop, long long id);
int aeProcessEvents(aeEventLoop *eventLoop, int flags);
int aeWait(int fd, int mask, long long milliseconds);
void aeMain(aeEventLoop *eventLoop);
char *aeGetApiName(void);
void aeSetBeforeSleepProc(aeEventLoop *eventLoop, aeBeforeSleepProc *beforesleep);
void aeSetAfterSleepProc(aeEventLoop *eventLoop, aeBeforeSleepProc *aftersleep);
int aeGetSetSize(aeEventLoop *eventLoop);
int aeResizeSetSize(aeEventLoop *eventLoop, int setsize);
void aeSetDontWait(aeEventLoop *eventLoop, int noWait);

#endif
