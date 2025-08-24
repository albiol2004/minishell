#ifndef JOBS_H
#define JOBS_H

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>

typedef struct {
    pid_t pid;
    int job_id;
    char command[1024];
    int status; // 1=activo, 2=bloqueado, 3=terminado
} Job;

void handle_sigchld();
void add_job(pid_t pid, const char *command);
void do_jobs();
void do_fg(int job_id);

#endif // JOBS_H