//
// Created by goksel on 11/26/22.
//

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "shmem.h"

#undef PRINT_LOG

#ifdef PRINT_LOG
#define LOG(...) printf(__VA_ARGS__)
#else
#define LOG(...)
#endif

char running = 1;
shmem_t* shmem;

void stop(){
    running = 0;
    pthread_cond_signal(&shmem->log_anomality_changed);
}

void sigint_handler(){
    LOG("SIGINT received, stopping...\n");
    pthread_t thread;
    pthread_create(&thread, NULL, (void*)stop, NULL);
    pthread_detach(thread);
}

int main(int argc, char *argv[]) {
    char* shmemaddr = argv[1];
    double threshold = strtod(argv[2], NULL);
    LOG("shmemaddr: %s\n", shmemaddr);
    LOG("threshold: %f\n", threshold);
    shmem = shmem_create(shmemaddr);
    signal(SIGINT, sigint_handler);
    while(running){
        LOG("Reporter locking mutex\n");
        pthread_mutex_lock(&shmem->logs_mutex);
        LOG("Reporter waiting for condition\n");
        pthread_cond_wait(&shmem->log_anomality_changed, &shmem->logs_mutex);
        if(!running){
            pthread_mutex_unlock(&shmem->logs_mutex);
            break;
        }
        LOG("Reporter making sure analyzers are done\n");
        LOG("Total log entries: %zu\n", shmem->entry_count);
        for(int i = 0; i < shmem->entry_count; i++){
            LOG("Locking %d\n", i);
            pthread_mutex_lock(&shmem->logs[i].mutex);
            LOG("Locked %d\n", i);
            LOG("Unlocking %d\n", i);
            pthread_mutex_unlock(&shmem->logs[i].mutex);
            LOG("Unlocked %d\n", i);
        }
        LOG("Reporter checking logs\n");
        for(int i = 0; i < shmem->entry_count;){
            //LOG("Checking %s\n", shmem->logs[i].logbody);

            if(shmem->logs[i].pending_analyzers > 0){
                i++;
                continue;
            }

            if(shmem->logs[i].anomality > threshold){
                printf("%5.2f :%s\n", shmem->logs[i].anomality, shmem->logs[i].logbody);
            }

            logentry_destroy(&shmem->logs[i]);
            shmem->entry_count--;
            for(int j = i; j < shmem->entry_count; j++) {
                shmem->logs[j] = shmem->logs[j + 1];
            }
            pthread_cond_broadcast(&shmem->log_deleted);

        }
        LOG("Reporter unlocked\n");
        pthread_mutex_unlock(&shmem->logs_mutex);
    }
    LOG("Reporter exiting\n");
    shmem_destroy(shmem);
    return 0;
}
