#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ncurses.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define SHM_SIZE 2000

typedef struct User {
    char username[20];
    char password[20];
} User;

int Crea_semaforo(key_t llave, int valor_inicial) {
    int semid = semget(llave, 1, IPC_CREAT | IPC_EXCL | 0666); // Agrega IPC_EXCL
    if (semid == -1) {
        return -1;
    }
    semctl(semid, 0, SETVAL, valor_inicial);
    return semid;
}

void down(int semid) {
    struct sembuf op_p[] = {0, -1, 0};
    semop(semid, op_p, 1);
}

void up(int semid) {
    struct sembuf op_v[] = {0, 1, 0};
    semop(semid, op_v, 1);
}

void cliente(int shmid, int semid) {
    char buffer[SHM_SIZE];
    User* shared_users = (User*)shmat(shmid, NULL, 0);

    User CurrentUser, ReadUser;
    char option;

    do {
        printw("Bienvenido al sistema de registro e inicio de sesion.\n"
               "Escoge una opcion para continuar\n"
               "1.- Inicio de sesion\n"
               "2.- Registro \n"
               "3.- Salir\n");

        option = getch();
        printw("\n");
        move(0, 0);
        clrtobot();

        if (option == '1') {
            down(semid); // Entrar a la sección crítica
            refresh();
            printw("Inicio de sesion\n\n Ingrese su nombre de usuario\n");

            refresh();
            getnstr(CurrentUser.username, 19);
            printw("Ingrese su contraseña\n");
            refresh();
            noecho();
            getnstr(CurrentUser.password, 19);
            refresh();
            echo();

            int already_regist = 0;

            for (int i = 0; i < SHM_SIZE / sizeof(User); ++i) {
                ReadUser = shared_users[i];
                if (strcmp(CurrentUser.username, ReadUser.username) == 0 && strcmp(CurrentUser.password, ReadUser.password) == 0) {
                    already_regist = 1;
                    break;
                }
            }

            if (already_regist == 1) {
                printw("Inicio de sesion exitoso \n");
            } else {
                printw("Error, intente otra vez. \n");
            }
            up(semid); // Salir de la sección crítica
        } else if (option == '2') {
            down(semid); // Entrar a la sección crítica
            refresh();
            printw("Registro de usuario\n\n Ingrese su nombre de usuario\n");

            refresh();
            getnstr(CurrentUser.username, 19);
            printw("Ingrese su contraseña\n");
            refresh();
            noecho();
            getnstr(CurrentUser.password, 19);
            refresh();
            echo();

            int already_regist = 0;

            for (int i = 0; i < SHM_SIZE / sizeof(User); ++i) {
                ReadUser = shared_users[i];
                if (strcmp(CurrentUser.username, ReadUser.username) == 0) {
                    already_regist = 1;
                    break;
                }
            }

            if (already_regist == 1) {
                printw("Ya habías sido registrado. \n");
            } else {
                printw("Registrando... \n");

                for (int i = 0; i < SHM_SIZE / sizeof(User); ++i) {
                    if (strlen(shared_users[i].username) == 0) {
                        shared_users[i] = CurrentUser;
                        break;
                    }
                }

                printw("Registro exitoso. \n");
            }
            up(semid); // Salir de la sección crítica
        } else if (option != '3') {
            refresh();
            printw("Opción no válida. Intente nuevamente.\n");
        }

        printw("Presione enter para volver al menú.\n");
        getch();
        move(0, 0);
        clrtobot();
    } while (option != '3');

    shmdt(shared_users);
}

int main() {
    initscr();

    key_t key = ftok("shmfile", 65);
    int shmid = shmget(key, SHM_SIZE, 0666 | IPC_CREAT);

    if (shmid == -1) {
        printw("Error al obtener la memoria compartida.\n");
        endwin();
        return 1;
    }

    int semid = Crea_semaforo(key, 1);
    if (semid == -1) {
        printw("Error al crear el semáforo.\n");
        endwin();
        return 1;
    }

    cliente(shmid, semid);

    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID); // Eliminar el semáforo

    endwin();
    return 0;
}
