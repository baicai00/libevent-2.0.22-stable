/*
 * Copyright (c) 2000-2007 Niels Provos <provos@citi.umich.edu>
 * Copyright (c) 2007-2012 Niels Provos and Nick Mathewson
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _EVENT2_EVENT_STRUCT_H_
#define _EVENT2_EVENT_STRUCT_H_

/** @file event2/event_struct.h

  Structures used by event.h.  Using these structures directly WILL harm
  forward compatibility: be careful.

  No field declared in this file should be used directly in user code.  Except
  for historical reasons, these fields would not be exposed at all.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <event2/event-config.h>
#ifdef _EVENT_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef _EVENT_HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

/* For int types. */
#include <event2/util.h>

/* For evkeyvalq */
#include <event2/keyvalq_struct.h>

#define EVLIST_TIMEOUT	0x01
#define EVLIST_INSERTED	0x02
#define EVLIST_SIGNAL	0x04
#define EVLIST_ACTIVE	0x08
#define EVLIST_INTERNAL	0x10
#define EVLIST_INIT	0x80

/* EVLIST_X_ Private space: 0x1000-0xf000 */
#define EVLIST_ALL	(0xf000 | 0x9f)

/* Fix so that people don't have to run with <sys/queue.h> */
#ifndef TAILQ_ENTRY
#define _EVENT_DEFINED_TQENTRY
#define TAILQ_ENTRY(type)						\
struct {								\
	struct type *tqe_next;	/* next element */			\
	struct type **tqe_prev;	/* address of previous next element */	\
}
#endif /* !TAILQ_ENTRY */

#ifndef TAILQ_HEAD
#define _EVENT_DEFINED_TQHEAD
#define TAILQ_HEAD(name, type)			\
struct name {					\
	struct type *tqh_first;			\
	struct type **tqh_last;			\
}
#endif

struct event_base;
struct event {
	/////////////////////////////////////////////////////////////
	TAILQ_ENTRY(event) ev_active_next;
	/* TAILQ_ENTRY(event)宏展开后如下所示--add by dengkai
	struct {
		struct event *tqe_next;
		struct event **tqe_prev;
	} ev_active_next;
	*/ 
	//////////////////////////////////////////////////////////////
	TAILQ_ENTRY(event) ev_next;
	/* for managing timeouts */
	union {
		TAILQ_ENTRY(event) ev_next_with_common_timeout;
		int min_heap_idx;
	} ev_timeout_pos;
	/*
	对于IO类型的事件，ev_fd保存对应的文件描述符(文件的fd或者socket套接字).
	对于信号事件，ev_fd保存具体的信号值(例如SIGINT等)
	对于定时器事件，ev_fd的值为-1
	 */
	evutil_socket_t ev_fd;

	struct event_base *ev_base;

	union {
		/* used for io events */
		struct {
			TAILQ_ENTRY(event) ev_io_next;
			struct timeval ev_timeout;// 保存超时时间(时间间隔)
		} ev_io;

		/* used by signal events */
		struct {
			TAILQ_ENTRY(event) ev_signal_next;
			/*
			ev_ncalls表示信号事件就绪时回调函数被调用的次数，假如该值为2，则回调函数将被连续调用两次。
			但是event_base的event_break成员为true时，将会中断调用，即ev_ncalls为2，而当前只调用了一次，然后检查event_break为true,则第2次调用将不会进行
			 */
			short ev_ncalls;
			/* Allows deletes in callback */
			short *ev_pncalls;// 指向ev_ncalls成员或者NULL(当ev_ncalls为零或者event_break为true时指向NULL)
		} ev_signal;
	} _ev;

	short ev_events;// 保存事件的标志，取值为：EV_TIMEOUT、EV_READ、EV_WRITE、EV_SIGNAL、EV_PERSIST、EV_ET
	short ev_res;		/* result passed to event callback */
	/*
	ev_flags应该是事件的状态标志，取值为EVLIST_XX宏，初始值为EVLIST_INIT
	 */
	short ev_flags;
	ev_uint8_t ev_pri;	/* smaller numbers are higher priority */
	/*
	目前ev_closure的取值有三种：EV_CLOSURE_SIGNAL、EV_CLOSURE_PERSIST、EV_CLOSURE_NONE，该成员在event_assign函数中被赋值，分三种情况：
	1.如果event_assign的events参数中包含EV_SIGNAL标志，则ev_closure被赋值为EV_CLOSURE_SIGNAL
	2.否则，如果events参数中包含EV_PERSIST标志，则被赋值为EV_CLOSURE_PERSIST
	3.否则，被赋值EV_CLOSURE_NONE
	 */
	ev_uint8_t ev_closure;
	struct timeval ev_timeout;//保存事件的超时时间点,而不是时间间隔

	/* allows us to adopt for different types of events */
	void (*ev_callback)(evutil_socket_t, short, void *arg); // 事件回调函数
	void *ev_arg; // 事件回调函数的参数
};

TAILQ_HEAD (event_list, event);
// event_list通过TAILQ_HEAD宏定义，具体如下所示, add by dengkai
/*
struct event_list {
	struct event *tqh_first;
	struct event **tqh_last;
};
*/

#ifdef _EVENT_DEFINED_TQENTRY
#undef TAILQ_ENTRY
#endif

#ifdef _EVENT_DEFINED_TQHEAD
#undef TAILQ_HEAD
#endif

#ifdef __cplusplus
}
#endif

#endif /* _EVENT2_EVENT_STRUCT_H_ */
