//
// Created by goksel on 11/26/22.
//

#include "shmem.h"

#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

shmem_t* shmem_create(const char *name){
    if(strlen(name) > sizeof((shmem_t*)(0))->name){
        perror("name too long");
    }
    int fd = shm_open(name, O_CREAT | O_EXCL | O_RDWR, 0666);
    if(fd == -1){
        shm_unlink(name);
        fd = shm_open(name, O_CREAT | O_EXCL | O_RDWR, 0666);
    }
    if(fd == -1){
        perror("shm_open");
    }
    if(ftruncate(fd, sizeof(shmem_t)) == -1){
        perror("ftruncate");
    }
    shmem_t *shmem = mmap(NULL, sizeof(shmem_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(shmem == MAP_FAILED){
        perror("mmap");
    }
    close(fd);

    memset(shmem, 0, sizeof(shmem_t));

    pthread_mutexattr_t mutexattr;
    pthread_mutexattr_init(&mutexattr);
    pthread_mutexattr_setpshared(&mutexattr, PTHREAD_PROCESS_SHARED);

    pthread_condattr_t condattr;
    pthread_condattr_init(&condattr);
    pthread_condattr_setpshared(&condattr, PTHREAD_PROCESS_SHARED);

    pthread_mutex_init(&shmem->logs_mutex, &mutexattr);
    pthread_mutex_init(&shmem->process_mutex, &mutexattr);
    pthread_cond_init(&shmem->log_added, &condattr);
    pthread_cond_init(&shmem->log_deleted, &condattr);
    pthread_cond_init(&shmem->log_anomality_changed, &condattr);

    strcpy(shmem->name, name);
    shmem->entry_count = 0;
    shmem->num_loggers = 0;
    shmem->num_analyzers = 0;

    pthread_mutexattr_destroy(&mutexattr);
    pthread_condattr_destroy(&condattr);

    return shmem;
}

shmem_t* shmem_get(const char *name){
    if(strlen(name) > sizeof((shmem_t*)(0))->name){
        perror("name too long");
    }
    int fd = shm_open(name, O_RDWR, 0666);
    if(fd == -1){
        perror("shm_open");
    }
    shmem_t *shmem = mmap(NULL, sizeof(shmem_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(shmem == MAP_FAILED){
        perror("mmap");
    }
    return shmem;
}

unsigned long shmem_get_timestamp(shmem_t *shmem){
    pthread_mutex_lock(&shmem->timestamp_mutex);
    unsigned long timestamp = shmem->timestamp++;
    pthread_mutex_unlock(&shmem->timestamp_mutex);
    return timestamp;
}

void shmem_detach(shmem_t *shmem){
    if(munmap(shmem, sizeof(shmem_t)) == -1){
        perror("munmap");
    }
}

void shmem_destroy(shmem_t *shmem){
    char* name = strdup(shmem->name);
    if(munmap(shmem, sizeof(shmem_t)) == -1){
        perror("munmap");
    }
    if(shm_unlink(name) == -1){
        perror("shm_unlink");
    }
    free(name);
}