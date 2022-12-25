//
// Created by goksel on 11/26/22.
//

#ifndef HW1_SHMEM_H
#define HW1_SHMEM_H

#include <pthread.h>
#include "logentry.h"
#include "limits.h"

#define MAX_LOG_ENTRIES 10000

typedef struct shmem {
    char name[4096];
    pthread_mutex_t logs_mutex;
    pthread_mutex_t process_mutex;
    pthread_mutex_t timestamp_mutex;
    pthread_cond_t log_added;
    pthread_cond_t log_deleted;
    pthread_cond_t log_anomality_changed;
    size_t entry_count;
    size_t num_loggers;
    size_t num_analyzers;
    unsigned long timestamp;
    logentry_t logs[MAX_LOG_ENTRIES];
} shmem_t;

shmem_t *shmem_create(const char *name);

void shmem_detach(shmem_t *shmem);

void shmem_destroy(shmem_t *shmem);

shmem_t *shmem_get(const char *name);

unsigned long shmem_get_timestamp(shmem_t *shmem);

#endif //HW1_SHMEM_H
