#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <limits.h>

//error handling macro
#define ERR(source) (fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
                     perror(source),kill(0,SIGKILL),\
                     exit(EXIT_FAILURE))

#define MAX_SIZE PIPE_BUF-8
volatile sig_atomic_t last_signal = 0;

int sethandler( void (*f)(int), int sigNo) {
        struct sigaction act;
        memset(&act, 0, sizeof(struct sigaction));
        act.sa_handler = f;

        if (-1==sigaction(sigNo, &act, NULL))
                return -1;

        return 0;
}

void signal_catch(int sig){
    last_signal=sig; 
}

typedef struct data{                                    //data structure saving buffer info to be sent
    int size;
    char buff[PIPE_BUF];
}data;


void usage(char * name){
        fprintf(stderr,"USAGE: %s t[50-500] n[3-30] r[0-100] b[1-%d]  \n",name,MAX_SIZE);
        exit(EXIT_FAILURE);
}

void childc_work(int p,int n,int t,int b){              //work performed by child c
    data d;

    srand(time(0)*getpid());

    for(int i=0;i<n;i++){

        if(last_signal==SIGINT) break;

        d.size=b+rand()%(MAX_SIZE+1-b);
        strcpy(d.buff,"");

        for(int j=0;j<d.size;j++){
            char c='a'+rand()%('z'+1-'a');
            strncat(d.buff,&c,1);
        } 

        struct timespec t1={0,t*1000000};
        if(TEMP_FAILURE_RETRY(write(p,&d,sizeof(data)))<0) ERR("write()");
        nanosleep(&t1,0);

    }
}

void childm_work(int p1,int p2,int r){                  //work performed by child m
    int status;
    data d;

    srand(time(0)*getpid());

    for(;;){
        strcpy(d.buff,"");

        status=TEMP_FAILURE_RETRY(read(p1,&d,sizeof(data)));
        if(status<0) ERR("read()");
        if(status==0) return;

        if(rand()%101<=r){
            strcat(d.buff,"injected");
            d.size=d.size+8;
        }
        if(TEMP_FAILURE_RETRY(write(p2,&d,sizeof(data)))<0) ERR("write()");
    }

}


void creating_children(int* pipe1,int n,int t,int b,int r){         //function which creates children
    int pipe2[2];

    for(int i=0;i<2;i++){
        switch (fork()){
        case 0:{
            if(pipe(pipe2)) ERR("pipe2");
            if(TEMP_FAILURE_RETRY(close(pipe1[0]))<0) ERR("close()");

            switch (fork()){

            case 0:
                sethandler(signal_catch,SIGINT);
                if(TEMP_FAILURE_RETRY(close(pipe2[0]))<0) ERR("close()");
                childc_work(pipe2[1],n,t,b);
                if(TEMP_FAILURE_RETRY(close(pipe2[1]))<0) ERR("close()");
                exit(EXIT_SUCCESS);
                break;
            
            case -1: ERR("fork()");
            }
            if(TEMP_FAILURE_RETRY(close(pipe2[1])<0)) ERR("close()");
            childm_work(pipe2[0],pipe1[1],r);
            if(TEMP_FAILURE_RETRY(close(pipe2[0]))<0) ERR("close()");
            if(TEMP_FAILURE_RETRY(close(pipe1[1]))<0) ERR("close()");

            waitpid(-1,NULL,0);
            exit(EXIT_SUCCESS);
        }    
            break;
        
        case -1: ERR("fork()");
        }
    }
    if(TEMP_FAILURE_RETRY(close(pipe1[1]))<0) ERR("close()");
}

int main(int argc, char** argv){
    int t,n,r,b;
    int pipe1[2];

    if(argc!=5) usage(argv[0]);

    t=atoi(argv[1]);
    n=atoi(argv[2]);
    r=atoi(argv[3]);
    b=atoi(argv[4]);

    if(t<50||t>500||n<3||n>30||r<0||r>100||b<1||b>MAX_SIZE){
        fprintf(stderr,"One of the values is incorrect\n");
        usage(argv[0]);
    }

    sethandler(SIG_IGN,SIGINT);

    if(pipe(pipe1)) ERR("pipe()");
    creating_children(pipe1,n,t,b,r);

    int status;
    data d;
    int i=0;

    for(;;){
        strcpy(d.buff,"");
        status=TEMP_FAILURE_RETRY(read(pipe1[0],&d,sizeof(data)));
        if(status<0) ERR("read()");
        else if(status==0) break;
        i++;
       printf("[%d]: [%d]: [%s]\n",i,d.size,d.buff);
    }

    if(TEMP_FAILURE_RETRY(close(pipe1[0]))<0) ERR("close()");
    waitpid(-1,NULL,0);

    return EXIT_SUCCESS;
}