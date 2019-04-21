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
// 屏障标记，当读写事件都就绪时：
// 如果没有设置屏障标记，读处理在写处理之前;
// 如果设置了屏障标记，读处理在写处理之后
// 在需要在发送响应之前记录日志时会用到
#define AE_BARRIER 4    /* With WRITABLE, never fire the event if the
                           READABLE event already fired in the same event
                           loop iteration. Useful when you want to persist
                           things to disk before sending replies, and want
                           to do that in a group fashion. */

// 是否处理IO事件的标记
#define AE_FILE_EVENTS 1
// 是否处理定时事件的标记
#define AE_TIME_EVENTS 2
#define AE_ALL_EVENTS (AE_FILE_EVENTS|AE_TIME_EVENTS)
// 确定多路复用IO调用是否会阻塞的标记
#define AE_DONT_WAIT 4
#define AE_CALL_AFTER_SLEEP 8

#define AE_NOMORE -1
#define AE_DELETED_EVENT_ID -1

/* Macros */
#define AE_NOTUSED(V) ((void) V)

struct aeEventLoop;

/* Types and data structures */
typedef void aeFileProc(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask);
// 定时事件的处理函数类型，
// 返回值如果不是-1（AE_NOMORE），则事件会继续触发，返回值是触发时间间隔
typedef int aeTimeProc(struct aeEventLoop *eventLoop, long long id, void *clientData);
typedef void aeEventFinalizerProc(struct aeEventLoop *eventLoop, void *clientData);
typedef void aeBeforeSleepProc(struct aeEventLoop *eventLoop);

/* File event structure */
// 感觉aeFiredEvent才是事件对象，这个称作事件处理器更合适一点，或者事件注册对象，
// 两个函数指针处理aeFiredEvent事件，注册IO事件时创建
typedef struct aeFileEvent {
    // 事件类型（读/写）
    int mask; /* one of AE_(READABLE|WRITABLE|BARRIER) */
    // 事件处理函数
    aeFileProc *rfileProc;
    aeFileProc *wfileProc;
    // 自定义数据对象，注册事件时传入，作为事件处理函数的参数
    void *clientData;
} aeFileEvent;

/* Time event structure */
typedef struct aeTimeEvent {
    // 定时事件标识Id，在EventLoop里唯一
    long long id; /* time event identifier. */
    // 定时事件触发的事件（绝对时间）
    long when_sec; /* seconds */
    long when_ms; /* milliseconds */
    // 定时事件的处理函数，返回值可以决定事件是否需要再次触发以及再次触发的时间间隔
    aeTimeProc *timeProc;
    // 事件节点被销毁之前会调用一次，作用应该和C++析构函数类似
    aeEventFinalizerProc *finalizerProc;
    // 自定义数据对象，注册事件时传入，作为前两个处理函数的参数
    void *clientData;
    // 链表指针
    struct aeTimeEvent *prev;
    struct aeTimeEvent *next;
} aeTimeEvent;

/* A fired event */
// 直观上没看懂这个fired是什么意思，看实现是已就绪的意思，对就绪IO事件的封装，
// 从不同的IO API（select/epoll...）得到的已就绪的IO事件都会转化成这个类型
// 看.c文件的注释，fire应该理解为消化/处理
typedef struct aeFiredEvent {
    // IO事件的文件描述符
    int fd;
    // IO事件类型（可读、可写）
    int mask;
} aeFiredEvent;

/* State of an event based program */
typedef struct aeEventLoop {
    // 当前文件描述符最大的值，初始为-1代表还没有监听的描述符，
    // 对应的是events和fired数组的最大有效索引，maxfd必然小于setsize
    // 从功能上讲maxfd不是必要的，维护这个字段应该是为了文件描述符较少时可以减少遍历次数，
    // 提升性能
    int maxfd;   /* highest file descriptor currently registered */
    // 监听的文件描述符最大个数，实际上就是events和fired两个数组的大小
    int setsize; /* max number of file descriptors tracked */
    // 定时事件的Id生成的参照，从0开始递增，生成的Id在同一个EventLoop里唯一
    long long timeEventNextId;
    // 记录上一次定时任务处理的时间，用来检测是否有系统时间的回退（系统时间可能被人为设置，
    // 或者通过网络重新校正），系统时间的回退可能导致定时事件的处理延迟
    time_t lastTime;     /* Used to detect system clock skew */
    // 数组类型，代表已注册的事件处理器，下标和文件描述符是对应的
    aeFileEvent *events; /* Registered events */
    // 数组类型，代表已就绪的IO事件，这里并没有存fired数组的有效长度，
    // 只在Process函数处理期间有效，缓存就绪的IO事件（随后调用events处理器消化事件）
    aeFiredEvent *fired; /* Fired events */
    // 双向（没有循环）链表结构，查找操作是O(n)，没有使用更高效的数据结构，
    // 大概目前不是瓶颈吧
    aeTimeEvent *timeEventHead;
    // 事件循环退出条件，如果设置了这个标记，事件循环结束
    int stop;
    // 不同系统支持的多路复用IO的Api不一样，对应需要的数据结构也不一样，所以下面是void*类型，
    // 对应的是IO事件的数据结构
    void *apidata; /* This is used for polling API specific data */
    // 下面两个回调函数分别在等待IO事件之前和之后调用，不过看实现两者的调用次数不一定相同
    aeBeforeSleepProc *beforesleep;
    aeBeforeSleepProc *aftersleep;
} aeEventLoop;

/* Prototypes */
// 创建事件循环对象，setsize是初始文件描述符数组的大小
aeEventLoop *aeCreateEventLoop(int setsize);
// 释放事件循环对象
void aeDeleteEventLoop(aeEventLoop *eventLoop);
// 设置事件循环结束标记
void aeStop(aeEventLoop *eventLoop);
// 添加监听的IO事件
// fd是IO事件的文件描述符
// mask是事件类型（读、写）
// proc是事件处理函数
// clientData是自定义数据对象，事件处理函数的额外参数
// 返回0（AE_OK）代表成功
int aeCreateFileEvent(aeEventLoop *eventLoop, int fd, int mask,
        aeFileProc *proc, void *clientData);
// 删除监听的IO事件
// mask是删除事件的类型，如果所有的事件类型都删除了，则文件描述符fd不再被监听
void aeDeleteFileEvent(aeEventLoop *eventLoop, int fd, int mask);
// 返回描述符上监听的事件类型
int aeGetFileEvents(aeEventLoop *eventLoop, int fd);
// 添加定时事件，在milliseconds毫秒后触发
// proc是事件处理函数
// clientData是自定义数据对象，事件处理函数的额外参数
// finalizerProc在时间被销毁前会调用一次
long long aeCreateTimeEvent(aeEventLoop *eventLoop, long long milliseconds,
        aeTimeProc *proc, void *clientData,
        aeEventFinalizerProc *finalizerProc);
// 删除定时事件
// 返回0（AE_OK）代表成功，如果事件不存在，则返回失败
int aeDeleteTimeEvent(aeEventLoop *eventLoop, long long id);
// 事件处理函数
// flags决定需要处理的事件类型（IO事件/定时事件）以及IO事件是否阻塞
int aeProcessEvents(aeEventLoop *eventLoop, int flags);
// 等待某个特定文件描述符的IO事件，返回就绪的事件类型
int aeWait(int fd, int mask, long long milliseconds);
// 事件循环入口函数
void aeMain(aeEventLoop *eventLoop);
char *aeGetApiName(void);
void aeSetBeforeSleepProc(aeEventLoop *eventLoop, aeBeforeSleepProc *beforesleep);
void aeSetAfterSleepProc(aeEventLoop *eventLoop, aeBeforeSleepProc *aftersleep);
// 获取事件循环文件描述符的最大个数
int aeGetSetSize(aeEventLoop *eventLoop);
// 重新设置事件循环文件描述符的最大个数
// 返回0（AE_OK）代表成功
// 如果缩减文件描述符上限导致不能容纳现有监听的文件描述符，则会返回失败
int aeResizeSetSize(aeEventLoop *eventLoop, int setsize);

#endif
