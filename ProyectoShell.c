/*------------------------------------------------------------------------------
Proyecto Shell de UNIX. Sistemas Operativos
Grados I. Informática, Computadores & Software
Dept. Arquitectura de Computadores - UMA

Algunas secciones están inspiradas en ejercicios publicados en el libro
"Fundamentos de Sistemas Operativos", Silberschatz et al.

Para compilar este programa: gcc ProyectoShell.c ApoyoTareas.c -o MiShell
Para ejecutar este programa: ./MiShell
Para salir del programa en ejecuciòn, pulsar Control+D

Manuel González Gamito, Computadores A
------------------------------------------------------------------------------*/

#include "ApoyoTareas.h"   // recordar compilar con el módulo job_control.c

#include <string.h>
#define MAX_LINE 256 /* 256 caracteres por línea, por comando, deberían ser suficientes. */

// -----------------------------------------------------------------------
job* tareas; // variable global para la lista de tareas


void manejador_sigchld(int sig) {
    int status, info, pid_wait; // DECLARO LAS VARIABLES
    enum status estado;
    job* trabajo = tareas;
    job *aux = NULL;
    block_SIGCHLD(); // bloquea las señales de los hijos 
    trabajo = trabajo->next;

    while (trabajo != NULL) {
        pid_wait = waitpid(trabajo->pgid, &status, WUNTRACED | WNOHANG); // pide la señal del pid seleccionado 
        estado = analyze_status(status, &info); // analiza el estado

        if (pid_wait == trabajo->pgid) { // si el pid del trabajo seleccionado coincide con el que ha mandado la señal 
            printf("Wait realizado a proceso en background: %s, pid: %i\n", trabajo->command, trabajo->pgid);
            if (estado == FINALIZADO || estado == SENALIZADO) {// analizamos el tipo de señal 
                aux = trabajo->next;
                delete_job(tareas, trabajo); // borramos el trabajo de la lista
                trabajo = aux; // seleccionamos el siguiente trabajo 
            }// FIN IF  
            else if (estado == SUSPENDIDO) {// si el estado es suspendido 
                trabajo->ground = DETENIDO; // paramos el trabajo 
                trabajo = trabajo->next; // seleccionamos el siguiente trabajo 
            }// FIN ELSE IF
        }// FIN IF 
        else {
            trabajo = trabajo->next;
        }// FIN ELSE
    }// FIN WHILE 
    unblock_SIGCHLD(); // desbloquea las señales de los hijos

}// FIN MANEJADOR

// -----------------------------------------------------------------------
//                            MAIN      
// -----------------------------------------------------------------------

int main(void) {



    char inputBuffer[MAX_LINE]; /* buffer para almacenar el comando ingresado */
    int background; /* vale 1 si un comando es seguido por '&' */
    char *args[MAX_LINE / 2]; /* línea de comando (de 256) tiene un máximo de 128 argumentos */
    // probablemente variables útiles:
    int pid_fork, pid_wait; /* pid para el proceso creado y esperado */
    int status; /* estado devuelto por wait */
    enum status status_res; /* estado procesado por analyze_status() */
    int info; /* info procesada por analyze_status() */
    int res;
    ignore_terminal_signals(); // ignoramos las señales del terminal 
    tareas = new_list("lista_trabajo"); // creamos la lista de trabajos 
    signal(SIGCHLD, manejador_sigchld); // leemos las señales de los hijos 


    while (1) {
        printf("COMANDO->");
        fflush(stdout);
        get_command(inputBuffer, MAX_LINE, args, &background); // leemos el comando


        if (args[0] == NULL) // comprobamos que la instrucción no esté vacía
        {
            continue;
        }

        if(!strcmp(args[0], "logout")){
          exit(0);
        }       		



        if (!strcmp(args[0], "cd")) { // comprobamos si es la instrucción interna cd
            
            if (args[1] == NULL) {// si no recibe parámetros
                chdir("/home");
            }
            int x=chdir(args[1]);
            if (x < 0) {// si recibe parámetros inválidos
                printf("No se puede cambiar al directorio %s\n", args[1]);
            }
            
            res = analyze_status(status, &info);
            
            continue;
        }


        if (!strcmp(args[0], "jobs")) { // generamos el comando interno jobs
            if (list_size(tareas) == 0) {
                printf("La lista está vacía \n");
            }
            else {
                print_job_list(tareas);
            } 
            continue;
        }


        if (strcmp(args[0], "fg") == 0) { // es el comando interno foreground (fg)
            int posicion;
            enum status statusfg;
            job * aux;
            if (args[1] == NULL) { // Si args[1] no existe, cogemos la pos 1
                posicion = 1;
            }
            else { // sino, la pos que le especifiquemos
                posicion = atoi(args[1]);
            }
            aux = get_item_bypos(tareas, posicion);
            if (aux == NULL) {
                printf("FG ERROR: trabajo no encontrado \n");
                continue;
            } 
            if (aux->ground == DETENIDO || aux->ground == SEGUNDOPLANO) {
                printf("Puesto en foreground el trabajo %d que estaba detenido o en background, el trabajo era: %s\n", posicion, aux->command);
                aux->ground = PRIMERPLANO; // cambiamos el estado de aux a foreground
                set_terminal(aux->pgid); // le damos el terminal
                killpg(aux->pgid, SIGCONT); 
                waitpid(aux->pgid, &status, WUNTRACED); // esperamos por el hijo 
                set_terminal(getpid()); // el padre recupera el terminal
                statusfg = analyze_status(status, &info);
                if (statusfg == SUSPENDIDO) {// si está suspendido lo paramos 
                    aux->ground = DETENIDO;
                }
                else {
                    delete_job(tareas, aux); // borramos el trabajo 
                }
            }
            else {
                printf("El proceso no estaba en background o detenido");
            }
            continue;
        }


        if (strcmp(args[0], "bg") == 0) { 
            int posicion;
            job * aux;
            if (args[1] == NULL) { 
                posicion = 1;
            }
            else {
                posicion = atoi(args[1]); // sino, la pos que le especifiquemos
            }
            aux = get_item_bypos(tareas, posicion); // seleccionamos el trabajo en la posición seleccionada
            if (aux == NULL) {
                printf("BG ERROR: trabajo no encontrado \n");
            } else if (aux->ground == DETENIDO) {// si está detenido lo pasamos a background					
                aux->ground = SEGUNDOPLANO;
                printf("Puesto en background el trabajo %d que estaba detenido, el trabajo era: %s\n", posicion, aux->command);
                killpg(aux->pgid, SIGCONT);
            } 
            continue;
        }

        pid_fork = fork(); // creamos un proceso hijo guardando su pid en pid_fork

                if (pid_fork < 0) { // si pid es negativo es que no se ha creado el hijo
                  perror("Fork erróneo"); 
                  exit(-1);
                  continue;
                }


        if (pid_fork == 0) { // el proceso hijo tiene pid 0
            new_process_group(getpid()); // el hijo crea su propio grupo de procesos
            if (background == 0) { 
                set_terminal(getpid()); // le damos el terminal al hijo
            }
            restore_terminal_signals(); // restauramos las señales para el hijo

            if (execvp(args[0], args) == -1) { // ejecutamos el comando
                printf("%s: No se encontró la orden\n", args[0]);
                exit(EXIT_FAILURE); 
            }
        } else {


            new_process_group(pid_fork); // el padre pone al hijo en un nuevo grupo de procesos

            if (background == 0) { 
                set_terminal(pid_fork); // le damos el terminal al hijo
                pid_wait = waitpid(pid_fork, &status, WUNTRACED); // esperamos por el hijo
                set_terminal(getpid()); // el padre recupera el terminal
                status_res = analyze_status(status, &info); // analizamos el estado del hijo
                printf("Foreground pid: %i, comando: %s, %s, info: %i\n", pid_fork, args[0], status_strings[status_res], info);
                if (status_res == SUSPENDIDO) { 
                    add_job(tareas, new_job(pid_fork, args[0], DETENIDO)); // lo añadimos a la lista de trabajos
                }
            } else { 
                printf("Background job running. pid: %i, command: %s\n", pid_fork, args[0]);
                add_job(tareas, new_job(pid_fork, args[0], SEGUNDOPLANO)); // lo añadimos a la lista de trabajos
            }
        }
    }
}
