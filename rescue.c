#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <stdbool.h>
#include <errno.h>

#define WORKDAY_MINUTES 540
#define NUM_STUDENTS 540

sem_t waitingRoom;
sem_t tutorTable;
sem_t studentsWaiting;
sem_t studentsEntered;
sem_t remainingStudentsMutex;
sem_t tutorDone;
bool isWorkdayEnded = false;

int waitingQueue[NUM_STUDENTS];
int frontOfQueue = 0;
int backOfQueue = 0;
int studentCount = 0;

sem_t studentCalled[NUM_STUDENTS];
sem_t studentDone[NUM_STUDENTS];
bool studentSelected[NUM_STUDENTS] = {false};

void* studentBehavior(void* arg) {
    int studentID = *((int*)arg);
    printf("Student %d says , \" Arriving at the student center . \"\n", studentID);


    

   sem_wait(&remainingStudentsMutex);
   if(isWorkdayEnded) {
    sem_post(&remainingStudentsMutex);
    return NULL;
   }
   if(sem_trywait(&waitingRoom) != 0){
    printf("Student %d says , \"There are no seats , guess I fail :-( \"\n", studentID);
    sem_post(&remainingStudentsMutex);
    return NULL;
   }
   printf("Student %d says , \" There is an empty seat , I will wait . \"\n", studentID);
   waitingQueue[backOfQueue++] = studentID - 1;

   sem_post(&remainingStudentsMutex);

   sem_wait(&studentCalled[studentID - 1]);

   sem_wait(&remainingStudentsMutex);
   if(!studentSelected[studentID - 1]) {
    sem_post(&remainingStudentsMutex);
    return NULL;
   }
   sem_post(&remainingStudentsMutex);

   sem_post(&studentsEntered);
   sem_wait(&studentDone[studentID - 1]);

   printf("Student %d says , \" Thank you , tutor ! \"\n", studentID);

   return NULL;

}



void* tutor_behavior(void* arg) {
    int numSeats = *((int*)arg);
    printf("Tutor says , \"I m beginning my workday , there are %d seats . \"\n", numSeats);
    while(1) {
        sem_wait(&remainingStudentsMutex);
       if(isWorkdayEnded) {
        while(frontOfQueue < backOfQueue) {
            int student = waitingQueue[frontOfQueue++];
            sem_post(&waitingRoom);
            sem_post(&studentCalled[student]);
        }
        sem_post(&remainingStudentsMutex);
        break;
       }
        
       if(backOfQueue - frontOfQueue < 2) {
        sem_post(&remainingStudentsMutex);
        printf("Tutor says , \" Not enough students , time to doomscroll . \"\n");
        sleep(1);
        continue;

       }

       int student1 = waitingQueue[frontOfQueue++];
       int student2 = waitingQueue[frontOfQueue++];

       studentSelected[student1] = true;
       studentSelected[student2] = true;

       sem_post(&waitingRoom);
       sem_post(&waitingRoom);
       sem_post(&remainingStudentsMutex);
       
       sem_post(&studentCalled[student1]);
       sem_post(&studentCalled[student2]);

        sem_wait(&studentsEntered);
        sem_wait(&studentsEntered);
        int minTutoringTime =5;
        int maxTutoringTime = 30;
        int tutoringTime = rand() % (maxTutoringTime - minTutoringTime + 1) + minTutoringTime;
        printf("Tutor says , \" There are two students , tutoring for %d minutes . \"\n", tutoringTime);
        sleep(tutoringTime);

        printf("tutor says , \" Tutoring done ! \"\n");

        sem_post(&studentDone[student1]);
        sem_post(&studentDone[student2]);
    
    
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
    if(numSeats < 1 || studentArrivalInterval < 1) {
        printf("invalid arguments.\n");
        return 1;
    }
    sem_init(&waitingRoom, 0, numSeats);
    sem_init(&tutorTable, 0, 0);
    sem_init(&studentsWaiting, 0, 0);
    sem_init(&studentsEntered, 0, 0);
    sem_init(&tutorDone, 0, 0);

    for(int i = 0; i < NUM_STUDENTS; i++) {
        sem_init(&studentCalled[i], 0, 0);
        sem_init(&studentDone[i], 0, 0);
    }
    sem_init(&remainingStudentsMutex, 0, 1);
    pthread_t students[NUM_STUDENTS];
    pthread_t tutor;
    int studentIDs[NUM_STUDENTS];
    pthread_create(&tutor, NULL, tutor_behavior, &numSeats);
    srand(time(NULL));

    for(int minute = 0; minute < WORKDAY_MINUTES; minute++) {
        if(rand() % studentArrivalInterval == 0) {
            studentIDs[studentCount] = studentCount + 1;

            pthread_create(&students[studentCount], NULL, studentBehavior, &studentIDs[studentCount]);
            studentCount++;
        }
        sleep(1);
    }
    sem_wait(&remainingStudentsMutex);
    isWorkdayEnded = true;
    sem_post(&remainingStudentsMutex);
    
   for(int i = 0; i < studentCount; i++) {
    pthread_join(students[i], NULL);
    }
    pthread_join(tutor, NULL);
    sem_destroy(&waitingRoom);
    sem_destroy(&tutorTable);
    sem_destroy(&studentsWaiting);
    sem_destroy(&studentsEntered);
    sem_destroy(&remainingStudentsMutex);
    sem_destroy(&tutorDone);
    for(int i = 0; i < NUM_STUDENTS; i++) {
        sem_destroy(&studentCalled[i]);
        sem_destroy(&studentDone[i]);
    }
    return 0;
}