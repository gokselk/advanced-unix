#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "shmem.h"

#ifdef PRINT_LOG
#define LOG(...) printf(__VA_ARGS__)
#else
#define LOG(...)
#endif

#define BUFFER_SIZE 512

char running = 1;
shmem_t *shmem;

void stop() {
    running = 0;
    pthread_cond_broadcast(&shmem->log_added);
}

void sigint_handler() {
    pthread_t thread;
    pthread_create(&thread, NULL, (void *) stop, NULL);
    pthread_detach(thread);
}

int main(int argc, char *argv[]) {
    char *shmemaddr = argv[1];
    fprintf(stderr, "shmemaddr: %s\n", shmemaddr);
    unsigned long port = strtoul(argv[2], NULL, 10);
    fprintf(stderr, "port: %lu\n", port);

    shmem = shmem_get(shmemaddr);

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("error opening socket");
        exit(1);
    }

    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    servaddr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        perror("error binding socket");
        exit(1);
    }

    char buffer[BUFFER_SIZE];
    struct sockaddr_in cliaddr;
    socklen_t len = sizeof(cliaddr);

    pthread_mutex_lock(&shmem->process_mutex);
    shmem->num_loggers++;
    pthread_mutex_unlock(&shmem->process_mutex);

    while (running) {
        ssize_t n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &cliaddr, &len);
        if (n < 0) {
            perror("error receiving message");
            exit(1);
        }
        LOG("received %s\n", buffer);
        buffer[n] = '\0';
        pthread_mutex_lock(&shmem->logs_mutex);
        LOG("locked logs_mutex\n");
        while (shmem->entry_count == MAX_LOG_ENTRIES) {
            pthread_cond_wait(&shmem->log_deleted, &shmem->logs_mutex);
            if (!running) {
                pthread_mutex_unlock(&shmem->logs_mutex);
                goto exit_loop;
            }
        }
        logentry_t entry;
        logentry_init(&entry, shmem, buffer);
        pthread_mutex_lock(&shmem->process_mutex);
        LOG("locked process_mutex\n");
        entry.pending_analyzers = shmem->num_analyzers;
        LOG("entry.pending_analyzers: %zu\n", entry.pending_analyzers);
        pthread_mutex_unlock(&shmem->process_mutex);
        LOG("inserting entry to %zu\n", shmem->entry_count);
        shmem->logs[shmem->entry_count++] = entry;
        pthread_mutex_unlock(&shmem->logs_mutex);
        pthread_cond_broadcast(&shmem->log_added);
        LOG("broadcasted log_added\n");
        pthread_cond_signal(&shmem->log_anomality_changed);
        LOG("signaled log_anomality_changed\n");
    }
    exit_loop:
    pthread_mutex_lock(&shmem->process_mutex);
    shmem->num_loggers--;
    pthread_mutex_unlock(&shmem->process_mutex);
    shmem_detach(shmem);
    close(sockfd);
    return 0;
}
