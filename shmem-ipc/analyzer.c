//
// Created by goksel on 11/26/22.
//

#include <stdlib.h>
#include <regex.h>
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include "shmem.h"

#ifdef PRINT_LOG
#define LOG(...) printf(__VA_ARGS__)
#else
#define LOG(...)
#endif

regex_t preg;
char mathop;
float mathval;
char running = 1;
shmem_t* shmem;

void logentry_process(logentry_t* entry){
    if(entry->pending_analyzers == 0){
        LOG("Analyzer %d is already done\n", (int)pthread_self());
        return;
    }
    //LOG("Processing %s\n", entry->logbody);
    char* logbody = entry->logbody;
    int reti = regexec(&preg, logbody, 0, NULL, 0);
    if(reti == REG_NOMATCH){
        entry->pending_analyzers--;
        return;
    }
    switch(mathop){
        case '+':
            entry->anomality += mathval;
            LOG("Anomality increased+ to %f\n", entry->anomality);
            break;
        case '*':
            entry->anomality *= mathval;
            LOG("Anomality increased* to %f\n", entry->anomality);
            break;
        default:
            break;
    }
    entry->pending_analyzers--;
}

ssize_t get_new_entry_index(unsigned long timestamp){
    ssize_t left = 0;
    ssize_t right = shmem->entry_count - 1;
    ssize_t result = -1;

    while(left <= right){
        ssize_t mid = (left+right)/2;
        if(shmem->logs[mid].timestamp > timestamp){
            result = mid;
            right = mid - 1;
        } else {
            left = mid + 1;
        }
    }
    return result;
}

void stop(){
    running = 0;
    pthread_cond_broadcast(&shmem->log_added);
}

void sigint_handler(){
    pthread_t thread;
    pthread_create(&thread, NULL, (void*)stop, NULL);
    pthread_detach(thread);
}

int main(int argc, char* argv[]){
    char* shmemaddr = argv[1];
    int rc;
    if((rc = regcomp(&preg, argv[2], REG_NOSUB)) != 0){
        char errbuf[1024];
        regerror(rc, &preg, errbuf, sizeof(errbuf));
        fprintf(stderr, "regex error: %s (%s)\n", errbuf, argv[2]);
        exit(1);
    }
    mathop = argv[3][0];
    mathval = strtof(argv[3]+1, NULL);
    shmem = shmem_get(shmemaddr);

    pthread_mutex_lock(&shmem->process_mutex);
    // Put current timestamp into last_processed_timestamp
    unsigned long last_processed_timestamp = shmem_get_timestamp(shmem);
    shmem->num_analyzers++;
    pthread_mutex_unlock(&shmem->process_mutex);

    signal(SIGINT, sigint_handler);

    while(running){
        //LOG("Last processed timestamp: %u\n", last_processed_timestamp);
        //LOG("trying to lock logs_mutex\n");
        pthread_mutex_lock(&shmem->logs_mutex);
        //LOG("checking for new log\n");
        ssize_t index = get_new_entry_index(last_processed_timestamp);
        if(index == -1){
            //LOG("no new log, waiting for log_added\n");
            pthread_cond_wait(&shmem->log_added, &shmem->logs_mutex);
            pthread_mutex_unlock(&shmem->logs_mutex);
            //LOG("log_added received, woke up\n");
            continue;
        }
        LOG("Processing entry at index %ld\n", index);
        pthread_mutex_lock(&shmem->logs[index].mutex);
        //LOG("locked entry mutex\n");
        pthread_mutex_unlock(&shmem->logs_mutex);
        //LOG("unlocked logs mutex\n");
        logentry_process(&shmem->logs[index]);
        //LOG("processed entry\n");
        last_processed_timestamp = shmem->logs[index].timestamp;
        if(shmem->logs[index].pending_analyzers == 0){
            LOG("A log is deleted\n");
            pthread_cond_signal(&shmem->log_anomality_changed);
        }
        pthread_mutex_unlock(&shmem->logs[index].mutex);
        //LOG("unlocked entry mutex\n");
    }

    pthread_mutex_lock(&shmem->process_mutex);
    shmem->num_analyzers--;
    pthread_mutex_unlock(&shmem->process_mutex);
    shmem_detach(shmem);
    regfree(&preg);
    return 0;
}
