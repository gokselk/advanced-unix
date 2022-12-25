//
// Created by goksel on 11/26/22.
//

#include <string.h>

#include <sys/time.h>
#include "logentry.h"
#include "shmem.h"

void logentry_init(logentry_t *entry, shmem_t *shmem, const char *content) {
    entry->anomality = 0;
    entry->pending_analyzers = 0;
    pthread_mutexattr_t mutexattr;
    pthread_mutexattr_init(&mutexattr);
    pthread_mutexattr_setpshared(&mutexattr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&entry->mutex, &mutexattr);
    pthread_mutexattr_destroy(&mutexattr);
    strcpy(entry->logbody, content);
    entry->timestamp = shmem_get_timestamp(shmem);
}

void logentry_destroy(logentry_t *entry) {
    pthread_mutex_destroy(&entry->mutex);
}
