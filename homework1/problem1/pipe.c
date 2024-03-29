#include <stdio.h>
#include <pthread.h>
#include <sched.h>
#include <stdlib.h>
#include <stdbool.h>

/************Shared memory*********/
volatile int in = 0;  //read_in for writing,
volatile int out = 0; //write_out for reading
volatile int pipe_is_closed = 0;
volatile int free_space = 0; //indicator for pipe_close to free pipe
volatile bool notFirstWrite = false;
volatile char *pipe;
int size_of_pipe;

//finish indicators for threads
volatile int thread_read_done=0;
volatile int thread_write_done=0;
/**********************************/

/**********Functions***************/
void pipe_init(int size);
void pipe_close();
void pipe_write(char write_byte);
int  pipe_read(char *read_byte);
bool isFull();
bool isEmpty();

void *thread_read();
void *thread_write();
/*********************************/

int main(int argc,char *argv[]){

    pthread_t thread1,thread2;
    int read_iret,write_iret;


    //check for paremeters
    if (argc<3){
        printf("Give K and N!\n");
        return(0);
    }

    size_of_pipe = atoi(argv[1]);

    //first create pipe
    pipe_init(size_of_pipe);

    //Create the reading thread
    read_iret= pthread_create(&thread1,NULL,thread_read,NULL);
    if (read_iret){
        fprintf(stderr,"Error - thread1() return code: %d\n",read_iret);
        exit(EXIT_FAILURE);
    }

    //Create the writing thread
    write_iret = pthread_create(&thread2,NULL,thread_write,NULL);
    if (write_iret){
        fprintf(stderr,"Error - thread2 return code: %d\n",write_iret);
        exit(EXIT_FAILURE);
    }

    //wait for thread to finish
    while(!thread_read_done){
        sched_yield();
    }
    while(!thread_write_done){
        sched_yield();
    }

    return(0);
}

//initialize pipe
void pipe_init(int size){
    pipe = (char *)calloc(size,sizeof(char));
    if (pipe==NULL){
        printf("Error-dynamic memory at %d line\n",__LINE__);
    }
}

//close pipe
void pipe_close(){
    pipe_is_closed = 1; // indicator that pipe is closed
    while(!free_space){}; //wait for read_pipe to read all bytes
    free((void*) pipe);
    thread_read_done=1; //notify main threads done 
    thread_write_done=1;
}

bool isFull(){
  bool full;
  if ((in + 1) % size_of_pipe == out){ full = true; }
  else{ full = false; }
  return full;
}

bool isEmpty(){
  bool empty;
  if (out == in){ empty = true; }
  else{ empty = false; }
  return empty;
}

void pipe_write(char write_byte) {
  while (isFull()){
    sched_yield();
  }
  pipe[in] = write_byte;
  in = (in + 1) % size_of_pipe;
}

int pipe_read(char *read_byte) {

  while (isEmpty()){
    if (pipe_is_closed == 1){
      return 0;
    }
    sched_yield();
  }

  *read_byte = pipe[out];
  out = (out + 1) % size_of_pipe;

  return(1);
}

void *thread_read(){

    char charRead;
    while(pipe_read(&charRead)){
        printf("%c",charRead);
    }

    //check for bytes in pipe after close
    while(in!=out){
        printf("%c",pipe[out]);
        out = (out + 1)% size_of_pipe;
    }
    
    free_space=1; //notify close that pipe_read done!
    return NULL;
}

void *thread_write(){

    char charWrite;
    charWrite = getchar();
    while (charWrite!=EOF){
        pipe_write(charWrite);
        charWrite = getchar();
    }

    pipe_close();
    return NULL;
}
