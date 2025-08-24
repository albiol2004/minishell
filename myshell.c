#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "parser.h"
#include <signal.h>
#include <errno.h>
#include "jobs.h"

int main(){
    char buf[1024];
    tline * line;
    int i;
    char* dir = getcwd(NULL, 0);
    printf("msh %s> ", dir);

    signal(SIGCHLD, handle_sigchld);

    while (fgets(buf, 1024, stdin)){
        line = tokenize(buf);
        if (line == NULL){
            continue;
        } 
        int status;
        pid_t pid;
        pid_t pids[line->ncommands]; // Array para gestionar los jobs y background
        int pipes[2][2]; // Usamos 2 pipes para alternar entre ellos
        int pipeactivo = 0; // Índice del pipe activo

        // Para lista de jobs
        char command_str[1024] = "";
        for (i = 0; i < line->ncommands; i++) {
            if (i > 0) {
                strcat(command_str, " | ");
            }
    
            // Por si acaso es NULL porque strcat no quiere funcionar y llevo dias buscando el segfault
            if (line->commands[i].filename != NULL) {
                strcat(command_str, line->commands[i].argv[0]);
            } else if (line->commands[i].argc > 0 && line->commands[i].argv[0] != NULL) {
                    strcat(command_str, line->commands[i].argv[0]);
            } else {
                strcat(command_str, "[unknown]");
            }
            for (int j = 1; j < line->commands[i].argc; j++) {
                if (line->commands[i].argv[j] != NULL) {
                    strcat(command_str, " ");
                    strcat(command_str, line->commands[i].argv[j]);
                }
            }
        }
        
        // COMANDOS
        for (i=0; i<line->ncommands; i++){
            //Comprobar si parser no ha encontrado el comando
            if (line->commands[i].filename == NULL) {
                if (strcmp(line->commands[i].argv[0], "cd") == 0){
                    if (chdir(line->commands[i].argv[1]) == -1){
                        perror("Error al cambiar de directorio");                    
                    }
                    free(dir);
                    dir = getcwd(NULL, 0);
                    break;    
                    // Comprobar jobs           
                } else if (strcmp(line->commands[i].argv[0], "jobs") == 0) {
                    do_jobs();
                    printf("msh %s > ", dir);
                    continue;
                } else if (strcmp(line->commands[i].argv[0], "fg") == 0) {
                    if (line->commands[0].argc < 2) {
                        fprintf(stderr, "fg: necesita un ID de job\n");
                    } else {
                        int job_id = atoi(line->commands[i].argv[1]);
                        do_fg(job_id);
                    }
                    printf("msh %s > ", dir);
                    continue;
                } else {
                    fprintf(stderr, "No existe el comando %s\n", line->commands[i].argv[0]);
                    continue;
                }
            }
            
            // Crear pipe para comandos intermedios
            if (i < line->ncommands - 1) {
                if (pipe(pipes[pipeactivo]) == -1) {
                    perror("Error creando pipe");
                    exit(1);
                }
            }

            pid = fork();
            if (pid == 0){ //HIJO
                signal(SIGINT, SIG_DFL);
                signal(SIGQUIT, SIG_DFL);
                if (line->redirect_input != NULL){ //INPUT PRIMER COMANDO
                    int fd = open(line->redirect_input, O_RDONLY);
                    if (fd == -1){
                        perror("Error al abrir el archivo de entrada");
                        exit(1);
                    }
                    if (dup2(fd, STDIN_FILENO) == -1) {
                        perror("Error en dup2 para entrada");
                        close(fd);
                        exit(1);
                    }
                    close(fd);
                }
                else if (i > 0) { // Entrada desde el pipe anterior para comandos no iniciales
                    if (dup2(pipes[1-pipeactivo][0], STDIN_FILENO) == -1) {
                        perror("Error en dup2 para pipe de entrada");
                        exit(1);
                    }
                }

 
                if (i == line->ncommands - 1) { // ULTIMO COMANDO
                    if (line->redirect_output != NULL) { // Redireccion
                        int fd = open(line->redirect_output, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        if (fd == -1) {
                            perror("Error al abrir el archivo de salida");
                            exit(1);
                        }
                        if (dup2(fd, STDOUT_FILENO) == -1) {
                            perror("Error en dup2 para salida");
                            close(fd);
                            exit(1);
                        }
                        close(fd);
                    }
                } else { // Intermedios
                    close(pipes[pipeactivo][0]); 
                    if (dup2(pipes[pipeactivo][1], STDOUT_FILENO) == -1) {
                        perror("Error en dup2 para pipe de salida");
                        exit(1);
                    }
                    close(pipes[pipeactivo][1]);
                }


                if (i == line->ncommands - 1 && line->redirect_error != NULL){ // ERROR ULTIMO COMANDO
                    int fd = open(line->redirect_error, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (fd == -1){
                        perror("Error al abrir el archivo de error");
                        exit(1);
                    }
                    if (dup2(fd, STDERR_FILENO) == -1) {
                        perror("Error en dup2 para error");
                        close(fd);
                        exit(1);
                    }
                    close(fd);
                }

                // Cerrar pipes intermedios
                if (i < line->ncommands - 1) {
                    close(pipes[pipeactivo][0]);
                    close(pipes[pipeactivo][1]);
                }
                if (i > 0) {
                    close(pipes[1-pipeactivo][0]);
                    close(pipes[1-pipeactivo][1]);
                }

                execvp(line->commands[i].filename, line->commands[i].argv);
                fprintf(stderr, "Error al ejecutar el comando: %s", strerror(errno));
                exit(1);
            }
            else if (pid > 0){ // PADRE
                signal(SIGINT, SIG_IGN);
                signal(SIGQUIT, SIG_IGN);
                pids[i]= pid;
                if (i > 0) {
                    close(pipes[1-pipeactivo][0]);
                    close(pipes[1-pipeactivo][1]);
                }
                
                // Cambiar el pipe activo
                pipeactivo = 1 - pipeactivo;
            }
            else {
                perror("Error en fork");
            }
        }
        
        if (!line->background) {
            signal(SIGCHLD, SIG_DFL); // Desactivado para foreground
            for (i = 0; i < line->ncommands; i++) {
                waitpid(pids[i], &status, 0);
            }
            signal(SIGCHLD, handle_sigchld);
        } else{
            for (int i = 0; i < line->ncommands; i++){
                if (line->commands[i].filename != NULL){
                    add_job(pids[line->ncommands-1], command_str);
                }

            }
        }
        fprintf(stdout, "msh %s > ", dir);
    }
    
    return 0;
}
