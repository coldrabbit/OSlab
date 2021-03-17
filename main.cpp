#include <QCoreApplication>
#include <stdio.h>
#include <unistd.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BUFFER_SIZE 64
#define BUFFER_NUM 6

int shmid[BUFFER_NUM];
int semid;
pid_t p1, p2, p3;

union semun{
    int val;               /* value for SETVAL */
    struct semid_ds *buf;  /* buffer for IPC_STAT, IPC_SET */
    unsigned short *array; /* array for GETALL, SETALL */
    struct seminfo *__buf; /* buffer for IPC_INFO */
}arg;

struct share_buf{
    int lenth;
    char buffer[BUFFER_SIZE];
    struct share_buf *next;
};

void P(int semid, int index){
    struct sembuf sem;
    sem.sem_num = index;
    sem.sem_op = -1;
    sem.sem_flg = 0;
    semop(semid, &sem, 1);
    return;
}

void V(int semid, int index){
    struct sembuf sem;
    sem.sem_num = index;
    sem.sem_op = 1;
    sem.sem_flg = 0;
    semop(semid, &sem, 1);
    return;
}

void get(){

}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    p1 = fork();
    if(p1 < 0){
        printf("fork child process 1 failed\n");
        exit(-1);
    }
    else if(p1 == 0){
        get();
    }
    else{
        p2 = fork();
        if(p2 < 0){
            printf("fork child process 2 failed\n");
            exit(-1);
        }
        else if(p2 == 0){
            trans();
        }
        else{
            p3 = fork();
            if(p3 < 0){
                printf("fork child process 3 failed\n");
                exit(-1);
            }
            else if(p3 == 0){
                put();
            }
            else{
                waitpid(p1, NULL, 0);
                            waitpid(p2, NULL, 0);
                            printf("child processes are killed\n");
                            semctl(semid, 0, IPC_RMID, arg);
                            semctl(semid, 1, IPC_RMID, arg);
                            printf("semid deleted\n");
                            for(i = 0; i < BUFFER_NUM; i++)
                                shmctl(shmid[i], IPC_RMID, 0);
                            printf("father process is killed\n");
            }
        }
    }

    return a.exec();
}
