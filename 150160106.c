/*
 *Mehmet Ali Han Tutuk
 *150160106
 *In Linux Mint terminal
 *Compile: gcc -Wall -Werror 150160106.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/stat.h>
// For creating semaphore different key needed
#define SEMKEY_INC 1
#define SEMKEY_TURN 2
#define SEMKEY_LOCK 3
#define SEMKEY_TURN2 4
#define SEMKEY_DEC 5
#define SEMKEY_MASTER 6


void mysigset(int num);
void mysignal(int signum);
void sem_wait(int semid, int val);
void sem_signal(int semid, int val);
int fib(int n);


int main(int argc, char **argv){
    if(argc<0){
        printf("Error!\n");
    }
    mysigset(SIGUSR2);
    int N = 150; // argv[1]
    int nI = 4;  // argv[2]
    int nD = 2;  // argv[3]
    int tI = 2;  // argv[4]
    int tD = 4;  // argv[5]
    int num_dec_processes=0;
// *************** Arrange semaphores and initalize their values ***********************
    int sem_inc,sem_dec,sem_turn,sem_turn2,sem_lock,sem_master;

    sem_inc=semget(SEMKEY_INC, 1, 0700|IPC_CREAT);
    semctl(sem_inc,0,SETVAL,1);  //initially inc semaphore =1
    sem_dec=semget(SEMKEY_DEC, 1, 0700|IPC_CREAT);
    semctl(sem_dec,0,SETVAL,0); //initially dec semaphore =0
    sem_turn=semget(SEMKEY_TURN, 1, 0700|IPC_CREAT);
    semctl(sem_turn,0,SETVAL,(nI*tI)); //for increaser process turn semaphore initially nI*tI
    sem_turn2=semget(SEMKEY_TURN2, 1, 0700|IPC_CREAT);
    semctl(sem_turn2,0,SETVAL,(nD*tD)); //for decreaser process turn semaphore initially nD*tD
    sem_lock=semget(SEMKEY_LOCK, 1, 0700|IPC_CREAT);
    semctl(sem_lock,0,SETVAL,1); //for shared memory and critical section
    sem_master=semget(SEMKEY_MASTER, 1, 0700|IPC_CREAT);
    semctl(sem_master,0,SETVAL,0); //for ending master

    int inc_turn=0,dec_turn=0; // for printing section
    int i,j,f=1;
    int segment_id;
    const int shared_segment_size=1; // create a shared memory for processes
    segment_id= shmget(IPC_PRIVATE,shared_segment_size,IPC_CREAT | IPC_EXCL |
                                                       S_IRUSR | S_IWUSR);
    int * shared_memory;
    shared_memory =(int *) shmat(segment_id,0,0); // create a shared memory
    *shared_memory=0;
    int *childs = (int *) malloc(sizeof(int)*(nI+nD));
    for(i = 0; i < (nI+nD); i++){ // create a child processes
        // Process created.
        f = fork();
        // Fork error
        if(f == -1){
            printf("Fork error...\n");
            exit(1);
        }
        // Child process
        if(f == 0)
            break;

        childs[i] = f;

    }

    if(f!=0){//parent
        printf("Master Process: Current money is %d\n",*shared_memory);
        sleep(1); // for printing section
        for(j=0;j<nI;j++){ // call increaser processes
            kill(childs[j],SIGUSR2);
        }

        for(j=0;j<nD;j++){ // call decreaser processes
            kill(childs[j+nI],SIGUSR2);

        }

        sem_wait(sem_master,1); // wait for decreaser process finish their work

        for(j=0;j<(nI+nD);j++){ // kill all child processes
            kill(childs[j],SIGKILL);
        }
        printf("Master Process: Killing all children and terminating the program\n");
        free(childs);
        shmdt(shared_memory);
        //// **** Destroy semaphores ***********
        semctl(sem_turn, 0, IPC_RMID);
        semctl(sem_turn2, 0, IPC_RMID);
        semctl(sem_inc, 0, IPC_RMID);
        semctl(sem_dec, 0, IPC_RMID);
        shmctl(sem_lock, 0, IPC_RMID);
        semctl(sem_master,0, IPC_RMID);

    }else if(f==0 && i<nI){ //increaser processes
        pause();
        int inc;
        if(i%2==0) inc=10;
        else inc =15;

        while(1){

            sem_wait(sem_inc,1);
            sem_signal(sem_inc,1);


                if(semctl(sem_turn,0,GETVAL,0) % nI == i && semctl(sem_turn,0,GETVAL,0) !=0 ){
                    sem_wait(sem_lock,1);
                    *shared_memory+=inc;
                    sem_wait(sem_turn,1);
                    if(semctl(sem_turn,0,GETVAL,0) % nI==0 && semctl(sem_turn,0,GETVAL,0) != nI*tI){
                        inc_turn++;
                        printf("Increaser Process %d: Current money is %d, increaser processes finished their turn %d\n",i,*shared_memory,inc_turn);
                    }else{
                        printf("Increaser Process %d: Current money is %d\n",i,*shared_memory);
                    }

                    sem_signal(sem_lock,1);
                }
                if(semctl(sem_turn,0,GETVAL,0)==0){
                    sem_wait(sem_lock,1);
                    if(0==num_dec_processes && *shared_memory < N){
                        semctl(sem_turn,0,SETVAL,(nI*tI));
                        num_dec_processes++;
                    }else{
                        if(semctl(sem_inc,0,GETVAL,0)!=0){
                            //printf("here %d sira dec\n",i);
                            sem_wait(sem_inc,1);
                            usleep(100); // for printing section
                            semctl(sem_turn2,0,SETVAL,nD*tD); //activate decreaser processes
                            sem_signal(sem_dec,1);  //activate decreaser processes
                        }

                    }
                    sem_signal(sem_lock,1);

                }

        }


    }else if(f==0 && i >= nI && i < (nI+nD)){ // decreaser processess
        pause();
        int cond;
        int dec;
        int fib_num=0;

        while (1){


           sem_wait(sem_dec,1);
           sem_signal(sem_dec,1);
           cond=0;

            if(semctl(sem_turn2,0,GETVAL,0) % nD == (i-nI) && semctl(sem_turn2,0,GETVAL,0) !=0 ){
                sem_wait(sem_lock,1);
                sem_wait(sem_turn2,1);


                if((i%2==0 ) == (*shared_memory%2==0)){
                    cond = 1;
                    fib_num++;
                }
                if(cond){
                    dec=fib(fib_num);
                    if(dec>=*shared_memory){
                        printf("Decreaser Process %d:Current money is less than %d signaling master to finish (%dth fibonaci number for decreaser %d)\n",i-nI,dec,fib_num-1,i-nI);
                        sem_signal(sem_master,1);
                        usleep(10000); //for synchronization
                    }
                    *shared_memory-=dec;

                }
                if(semctl(sem_turn2,0,GETVAL,0) % nD==0 && semctl(sem_turn2,0,GETVAL,0) != nD*tD){
                    dec_turn++;
                    printf("Decreaser Process %d: Current money is %d (%dst fibonacci number for decreaser %d), decreaser processes finished their turn %d\n",i-nI,*shared_memory,fib_num,(i-nI),dec_turn);
                    num_dec_processes++;
                }else{
                    printf("Decreaser Process %d: Current money is %d (%dst fibonacci number for decreaser %d)\n",i-nI,*shared_memory,fib_num,(i-nI));

                }
                sem_signal(sem_lock,1);
            }
            if(semctl(sem_turn2,0,GETVAL,0)==0){

                    sem_wait(sem_dec,1);
                    usleep(100); // for printing section
                    semctl(sem_turn,0,SETVAL,nI*tI); //activate increaser processes
                    sem_signal(sem_inc,1); //activate increaser processes

            }

        }

    }


    return 0;
}

int fib(int n){ // for finding n'th fibonacci number in decreaser processos

    int *array = (int*) malloc((n+2) * sizeof(int));
    int i;
    array[0] = 0; array[1] = 1;
    for (i = 2; i <= n; i++){
        array[i] = array[i-1] + array[i-2];
    }
    int result=array[n];
    free(array);
    return result;
}

void mysigset(int num){
    struct sigaction mysigaction;
    mysigaction.sa_handler = (void*) mysignal;
    mysigaction.sa_flags = 0;
    sigaction(num, &mysigaction, NULL);
}

// signal_handling function
void mysignal(int signum){
    //printf("Received signal with num=%d\n",signum );
}

// semaphore increment operation
void sem_signal(int semid, int val){
    struct sembuf semaphore;
    semaphore.sem_num = 0;
    semaphore.sem_op = val;
    semaphore.sem_flg = 1;	// relavive: add sem_op to value
    semop(semid, &semaphore, 1);
}

// semaphore decrement operation
void sem_wait(int semid, int val){
    struct sembuf semaphore;
    semaphore.sem_num = 0;
    semaphore.sem_op = (-1*val);
    semaphore.sem_flg = 1;	// relavive: add sem_op to value
    semop(semid, &semaphore, 1);
}
