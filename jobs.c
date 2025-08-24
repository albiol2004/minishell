#include "jobs.h"

#define MAX_JOBS 32

// Globales
static Job job_list[MAX_JOBS];
static int job_count = 0;
static int next_job_id = 1;

// SIGCHLD handler
void handle_sigchld() {
    pid_t pid;
    int status;
    
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        // Actualizando
        for (int i = 0; i < job_count; i++) {
            if (job_list[i].pid == pid) {
                if (WIFEXITED(status) || WIFSIGNALED(status)) {
                    job_list[i].status = 3; // hecho
                }
                break;
            }
        }
    }
}

void add_job(pid_t pid, const char *command) {
    if (job_count >= MAX_JOBS) {
        fprintf(stderr, "Error: La lista de jobs está llena\n");
        return;
    }
    
    // Actualizar lista
    int i = 0;
    while (i < job_count) {
        if (job_list[i].status == 3) {
            // Elimino por desplazamiento
            for (int j = i; j < job_count - 1; j++) {
                job_list[j] = job_list[j + 1];
            }
            job_count--;
        } else {
            i++;
        }
    }
    
    job_list[job_count].pid = pid;
    job_list[job_count].job_id = next_job_id++;
    job_list[job_count].status = 1; // Activo
    strncpy(job_list[job_count].command, command, sizeof(job_list[job_count].command) - 1);
    job_list[job_count].command[sizeof(job_list[job_count].command) - 1] = '\0';
    
    printf("[%d] %d\n", job_list[job_count].job_id, pid);
    job_count++;
}

// Mostrar
void do_jobs() {
    // Actualizar (habria estado bien un metodo actualizar eh)
    for (int i = 0; i < job_count; i++) {
        if (job_list[i].status != 3) { 
            int status;
            pid_t result = waitpid(job_list[i].pid, &status, WNOHANG);
            
            if (result == job_list[i].pid) {
                job_list[i].status = 3; 
            } else if (result == -1) {
                job_list[i].status = 3;
            }
        }
    }
    // Eliminar
    int i = 0;
    while (i < job_count) {
        if (job_list[i].status == 3) {

            for (int j = i; j < job_count - 1; j++) {
                job_list[j] = job_list[j + 1];
            }
            job_count--;
        } else {
            i++;
        }
    }
    
    // Los que queden
    if (job_count == 0) {
        printf("No hay trabajos en segundo plano\n");
        return;
    }
    
    printf("Jobs\n");
    printf("%-6s %-6s %-20s %s\n", "PID", "Job_id", "Status", "Command");
    printf("--------------------------------------------------\n");
    
    for (i = 0; i < job_count; i++) {
        printf("%-6d %-6d %-20s %s\n",
               job_list[i].pid,
               job_list[i].job_id,
               job_list[i].status == 1 ? "Activo" :
               job_list[i].status == 2 ? "Parado" : "Finalizado",
               job_list[i].command);
    }
}

// FG
void do_fg(int job_id) {
    int found = -1;
    
    // Buscar
    for (int i = 0; i < job_count; i++) {
        if (job_list[i].job_id == job_id) {
            found = i;
            break;
        }
    }
    
    if (found == -1) {
        fprintf(stderr, "fg: %d: no existe ese job\n", job_id);
        return;
    }
    
    pid_t pid = job_list[found].pid;
    printf("%s\n", job_list[found].command);
    
    // Seguir
    kill(pid, SIGCONT);
    
    int status;
    waitpid(pid, &status, WUNTRACED);
    
    for (int i = found; i < job_count - 1; i++) {
        job_list[i] = job_list[i + 1];
    }
    job_count--;
}