#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <sys/sem.h>
#include <string.h>

int semaforo;
char *guarda;

int Crea_semaforo(int llave, int valor_inicial) {
    int semid = semget(llave, 1, IPC_CREAT | 0666);
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

void Cliente() {
    char frase[100];
    while (1) {
        fgets(frase, sizeof(frase), stdin);
        down(semaforo);
        if (strcmp(frase, "salir\n") == 0) {
            break;
        }
        strncpy(guarda, frase, 100);
        up(semaforo);
    }
}

int main() {
    int llave, shmid;
    llave = ftok("Archivo", 'k');
    shmid = shmget(llave, 100 * sizeof(char), IPC_CREAT | 0777);
    guarda = (char *)shmat(shmid, 0, 0);
    semaforo = Crea_semaforo(llave, 1);

    Cliente();

    shmdt(guarda);
    return 0;
}

