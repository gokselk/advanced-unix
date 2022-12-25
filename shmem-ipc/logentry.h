//
// Created by goksel on 11/26/22.
//

#ifndef HW1_LOGENTRY_H
#define HW1_LOGENTRY_H

#include "pthread.h"
typedef struct shmem shmem_t;

typedef struct logentry {
    double anomality;
    char logbody[512];
    pthread_mutex_t mutex;
    size_t pending_analyzers;
    unsigned long timestamp;
} logentry_t;

void logentry_init(logentry_t *entry, shmem_t *shmem, const char *content);
void logentry_destroy(logentry_t *entry);

#endif //HW1_LOGENTRY_H
