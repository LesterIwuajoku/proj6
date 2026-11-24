#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <stdbool.h>
#include <errno.h>

#define NUM_STUDENTS 10

sem_t waitingRoom;
sem_t tutorTable;
sem_t studentsWaiting;
sem_t studentsEntered;
sem_t remainingStudentsMutex;

int remainingStudents = NUM_STUDENTS;

void* studentBehavior(void* arg) {
    int studentID = *((int*)arg);
    printf("Student %d says , \" Arriving at the student center . \"\n", studentID);

    if(sem_trywait(&waitingRoom) != 0) {
        printf("Student %d says , \"There are no seats , guess I fail :-( \"\n", studentID);
        sem_wait(&remainingStudentsMutex);
        remainingStudents--;
        sem_post(&remainingStudentsMutex);
        return NULL;
    }

    printf("Student %d says , \" There is an empty seat , I will wait . \"\n", studentID);
    sem_post(&studentsWaiting);
    sem_wait(&tutorTable);
    sem_post(&studentsEntered);

    return NULL;
}



void* tutor_behavior(void* arg) {
    printf("Tutor says , \"I m beginning my workday , there are 2 seats . \"\n");
    while(1) {
        sem_wait(&remainingStudentsMutex);
        if(remainingStudents <= 0) {
            sem_post(&remainingStudentsMutex);
            break;
        }
        
        sem_post( &remainingStudentsMutex);
        sem_wait( &studentsWaiting);
        sem_wait(&studentsWaiting);
        sem_post(&waitingRoom);
        sem_post(&waitingRoom);
        sem_post(&tutorTable);
        sem_post(&tutorTable);
        sem_wait(&studentsEntered);
        sem_wait(&studentsEntered);
        int minTutoringTime =5;
        int maxTutoringTime = 30;
        int tutoringTime = rand() % (maxTutoringTime - minTutoringTime + 1) + minTutoringTime;
        printf("Tutor says , \" There are two students , tutoring for %d minutes . \"\n", tutoringTime);
        sleep(tutoringTime);
        sem_wait(&remainingStudentsMutex);
        remainingStudents-= 2;
        sem_post(&remainingStudentsMutex);
    }
    printf("Tutor says , \" That's enough for one day , good night ! \"\n");
    return NULL;
}

int main(int argc, char* argv[]) {
    if(argc != 3) {
        printf("Usage: %s <capacity of waitingroom> <student arrival interval>\n", argv[0]);
        return 1;
    }
    int numSeats = atoi(argv[1]);
    int studentArrivalInterval = atoi(argv[2]);
    sem_init(&waitingRoom, 0, numSeats);
    sem_init(&tutorTable, 0, 0);
    sem_init(&studentsWaiting, 0, 0);
    sem_init(&studentsEntered, 0, 0);
    sem_init(&remainingStudentsMutex, 0, 1);
    pthread_t students[NUM_STUDENTS];
    pthread_t tutor;
    pthread_create(&tutor, NULL, tutor_behavior, NULL);
    for(int i = 1; i <= NUM_STUDENTS; i++) {
        pthread_create(&students[i-1], NULL, studentBehavior, &i);
        int nextArrival = rand() % studentArrivalInterval + 1;
        sleep(nextArrival);
    }
    
    for(int i = 0; i < NUM_STUDENTS; i++) {
        pthread_join(students[i], NULL);
    }
    pthread_join(tutor, NULL);
    sem_destroy(&waitingRoom);
    sem_destroy(&tutorTable);
    sem_destroy(&studentsWaiting);
    sem_destroy(&studentsEntered);
    sem_destroy(&remainingStudentsMutex);
    return 0;
}