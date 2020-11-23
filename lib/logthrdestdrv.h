/*
 * Copyright (c) 2013, 2014 Balabit
 * Copyright (c) 2013, 2014 Gergely Nagy <algernon@balabit.hu>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * As an additional exemption you are allowed to compile & link against the
 * OpenSSL libraries as published by the OpenSSL project. See the file
 * COPYING for details.
 *
 */

#ifndef LOGTHRDESTDRV_H
#define LOGTHRDESTDRV_H

#include "syslog-ng.h"
#include "driver.h"
#include "stats/stats-registry.h"
#include "logqueue.h"
#include "mainloop-worker.h"
#include <iv.h>
#include <iv_event.h>

typedef enum
{
  WORKER_INSERT_RESULT_DROP,
  WORKER_INSERT_RESULT_ERROR,
  WORKER_INSERT_RESULT_EXPLICIT_ACK_MGMT,
  WORKER_INSERT_RESULT_SUCCESS,
  WORKER_INSERT_RESULT_QUEUED,
  WORKER_INSERT_RESULT_NOT_CONNECTED
} worker_insert_result_t;

typedef struct _LogThreadedDestDriver LogThreadedDestDriver;
typedef struct _LogThreadedDestWorker
{
  LogQueue *queue;
  gboolean connected;
  void (*thread_init)(LogThreadedDestDriver *s);
  void (*thread_deinit)(LogThreadedDestDriver *s);
  gboolean (*connect)(LogThreadedDestDriver *s);
  void (*disconnect)(LogThreadedDestDriver *s);
  worker_insert_result_t (*insert)(LogThreadedDestDriver *s, LogMessage *msg);
  worker_insert_result_t (*flush)(LogThreadedDestDriver *s);
} LogThreadedDestWorker;

struct _LogThreadedDestDriver
{
  LogDestDriver super;
  struct iv_task  do_work;
  struct iv_event wake_up_event;
  struct iv_event shutdown_event;
  struct iv_timer timer_reopen;
  struct iv_timer timer_throttle;
  gboolean startup_finished;
  GCond *started_up;
  GMutex *lock;

  StatsCounterItem *dropped_messages;
  StatsCounterItem *queued_messages;
  StatsCounterItem *processed_messages;
  StatsCounterItem *written_messages;
  StatsCounterItem *memory_usage;

  gboolean suspended;
  gboolean under_termination;
  time_t time_reopen;
  gint batch_size;
  gint rewound_batch_size;
  gint retries_counter;
  gint retries_max;

  LogThreadedDestWorker worker;

  gint stats_source;
  gint32 seq_num;

  WorkerOptions worker_options;
  const gchar *(*format_stats_instance)(LogThreadedDestDriver *s);
};

void log_threaded_dest_driver_ack_messages(LogThreadedDestDriver *self, gint batch_size);
void log_threaded_dest_driver_drop_messages(LogThreadedDestDriver *self, gint batch_size);
void log_threaded_dest_driver_rewind_messages(LogThreadedDestDriver *self, gint batch_size);

gboolean log_threaded_dest_driver_deinit_method(LogPipe *s);
gboolean log_threaded_dest_driver_init_method(LogPipe *s);

void log_threaded_dest_driver_init_instance(LogThreadedDestDriver *self, GlobalConfig *cfg);
void log_threaded_dest_driver_free(LogPipe *s);

void log_threaded_dest_driver_set_max_retries(LogDriver *s, gint max_retries);

#endif
