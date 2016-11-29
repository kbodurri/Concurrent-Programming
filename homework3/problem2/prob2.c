#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>


#include "debug.h"

pthread_mutex_t mtx;
pthread_cond_t startTrain, embarkate, debarkate;
int nOfPassenger=0, N = 0, trainStarted = 0, trainFinished = 0;
int blockedEmbarkation = 0, blockedDebarkation = 0;

void *train();
void *passenger(void *arg);
void trainStart();
void trainStop();
void embarkation(int id);
void debarkation(int id);

int main(int argc,char *argv[]){

    pthread_t train_thread,passenger_thread;
    int iret,i,total_passengers,timing;
    int *id;

    if (argc<3){
        debug_e("Give N and total number of passengers");
        return(EXIT_FAILURE);
    }

    N = atoi(argv[1]);
    total_passengers = atoi(argv[2]);

    id = (int *) malloc(total_passengers * sizeof(int));

    debug("Init mtx at 1");
    pthread_mutex_init(&mtx, NULL);

    pthread_cond_init(&startTrain, NULL);
    pthread_cond_init(&embarkate, NULL);
    pthread_cond_init(&debarkate, NULL);

    iret = pthread_create(&train_thread,NULL,&train,NULL);
    if (iret){
        debug_e("pthread_create for train fail");
        return(EXIT_FAILURE);
    }

    for (i=0; i<total_passengers; i++){
        scanf("%d",&timing);
        sleep(timing);
        id[i] = i;
        iret = pthread_create(&passenger_thread,NULL,&passenger, (void *) (id + i));
        if (iret){
            debug_e("pthread_create for train fail");
            return(EXIT_FAILURE);
        }
    }

    pthread_join(train_thread,NULL);

    free(id);

    return (0);
}


void *train(){

    while(1){
        debug("Train is entering monitor to wait to be full");
        pthread_mutex_lock(&mtx);
        trainStart();
        debug("Train exits monitor");
        pthread_mutex_unlock(&mtx);
        debug("Train is about to begin");
        sleep(2);
        debug("Ride is over");
        debug("Train is entering monitor to empty");
        pthread_mutex_lock(&mtx);
        trainStop();
        pthread_mutex_unlock(&mtx); 
        if (trainStarted == 0){
            debug("Train is empty and exits monitor");
        }
        else{
            debug("Train exits monitor and passengers are debarkating");
        }
    }

    return NULL;
}

void *passenger(void *arg){
    int id;
    id = *(int *)arg;

    debug("Passenger %d is entering monitor to embarkate", id);
    pthread_mutex_lock(&mtx);
    debug("Passenger %d is going to embarkate", id);
    embarkation(id);
    debug("Passenger %d embarkated and exits the monitor", id);
    pthread_mutex_unlock(&mtx);
    //Passenger wait for train to finish its ride to debarkate
    debug("Passenger %d is entering monitor to debarkate", id);
    pthread_mutex_lock(&mtx);
    debug("Passenger %d is going to debarkate", id);
    debarkation(id);
    debug("Passenger %d debarkated and exits the monitor", id);
    pthread_mutex_unlock(&mtx);
    debug("Passenger %d is done", id);

    return NULL;
}

void trainStart(){
    //debug("trainStarted = %d || trainFinished = %d", trainStarted, trainFinished);
    if(trainStarted == 0 || trainFinished == 1){
        debug("Train waits to full");
        pthread_cond_wait(&startTrain, &mtx);
    }
    else {
        debug("Train is full. Ride is going to start");
    }
}

void trainStop(){
    trainFinished = 1;
    debug("Debarkation is about to begin");
    pthread_cond_signal(&debarkate);
}

void embarkation(int id){
    if(trainStarted == 1){
        /*Train is started.
        Other passengers must wait until train finish its ride. */
        blockedEmbarkation++;
        debug("Passenger %d waits for train to finish its ride", id);
        pthread_cond_wait(&embarkate, &mtx);
    }
    nOfPassenger++;
    debug("Passenger %d is on train. Total number of passengers: %d",id, nOfPassenger);
    if(nOfPassenger == N){
        /*Train is full.
        Notify train to start the ride */
        debug("Last passenger entered the train.Start the train");
        trainStarted = 1;
        pthread_cond_signal(&startTrain);
    }
    else{
        if(blockedEmbarkation != 0){
            blockedEmbarkation--;
            debug("Passenger %d wakes up other blocked passenger", id);
            pthread_cond_signal(&embarkate);
        }
    }
}

void debarkation(int id){
    //debug("Passenger %d waits for train to finish its ride %d", id);
    if(trainFinished == 0){
        blockedDebarkation++;
        debug("Passenger %d waits for train to finish its ride, to debarkate", id);
        pthread_cond_wait(&debarkate, &mtx);
    }

    nOfPassenger--;
    debug("Passenger %d debarkated. Remaining passengers: %d", id, nOfPassenger);
    if (nOfPassenger == 0){
        /*Last passenger out of the train.
        Notify other passenger to embarkate */
        debug("Last passenger (id = %d) debarkated", id);
        trainStarted = 0;
        if (blockedEmbarkation != 0){
            blockedEmbarkation--;
            trainFinished = 0;
            pthread_cond_signal(&embarkate);
        }
    }
    else{
        if (blockedDebarkation != 0) {
            blockedDebarkation--;
            pthread_cond_signal(&debarkate);
        }
    }
}
