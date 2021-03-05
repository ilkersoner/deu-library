#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#define ROOM_NUMBER 10
#define STUDENT_NUMBER 30
#define ROOM_CAPACITY 4


sem_t rooms[ROOM_NUMBER];
sem_t remainingStudent;

int roomStates[ROOM_NUMBER];
int roomStudentNumber[ROOM_NUMBER];
int waitingStudents = STUDENT_NUMBER;
int studyingStudents [ROOM_NUMBER][ROOM_CAPACITY];


void *student(void *id){

  int studentNumber = (int)id;
  sleep(rand() %10);
  int studentState = 2;

  while(1){
    if(studentState == 1){ //STATE 1 = STUDYING
      printf("Student %d is studying.\n",studentNumber);
      sleep(1);
      break;
    }
    else if(studentState == 2){ //STATE 2 = WAITING IN THE LIBRARY
      printf("Student %d is looking for an empty room.\n",studentNumber);
      int i = 0;
      int studentToStudy = 0;
      for(i = 0;i<ROOM_NUMBER;i++){
        sem_wait(rooms+i);
        if(roomStates[i] == 1){ // IF ROOM STATE IS EMPTY STUDENT GETS INTO THE ROOM
          printf("Student %d enters the room %d\n",studentNumber,i );
          roomStudentNumber[i]++;
          int a;
          for(a = 0;a<ROOM_CAPACITY;a++){ // LOOKING FOR AN EMPTY SEAT.STUDENT SITS TO THE EMPTY SEAT
            if(studyingStudents[i][a] == -1){
              studyingStudents[i][a] = studentNumber;
              break;
            }
          }
          if(roomStudentNumber[i] >= ROOM_CAPACITY){ //CHECKING IF ROOM STATE= FULL AND BUSY STATE.
            roomStates[i] = 3;
	    printf("Room %d is full and all the students are studying.\n",i);
          }

          studentToStudy = 1; //STUDENT IS STUDYING.
          studentState = 1;
          sem_post(rooms+i);
          break;
        }
        sem_post(rooms+i);
      }
      if(studentToStudy == 0){ //IF STUDENT IS NOT STUDYING
        for(i = 0;i<ROOM_NUMBER;i++){ //STUDENT LOOKS FOR A ROOM.
          sem_wait(rooms+i);
          if(roomStates[i] == 2){ //STUDENT FOUND A ROOM IN IDLE STATE
            printf("Student %d alerts keeper in room %d.Cleaning in room %d is finished.\n",studentNumber,i,i);
            roomStates[i] = 1;
            printf("Student %d gets into the room %d\n",studentNumber,i );
            roomStudentNumber[i]++;

            int b;
            for(b = 0;b<ROOM_CAPACITY;b++){ //LOOKING FOR AN EMPTY SEAT.STUDENT SITS ON THE EMPTY SEAT.
              if(studyingStudents[i][b] == -1){
                studyingStudents[i][b] = studentNumber;
                break;
              }
            }

            if(roomStudentNumber[i] >= ROOM_CAPACITY){
              roomStates[i] = 3;
            }
            studentToStudy = 1;
            studentState = 1;
            sem_post(rooms+i);
            break;
          }

          sem_post(rooms+i);
        }
      }
      if(studentToStudy == 0){
        sleep(rand()%5+2);
      }
    }
  }
  return 0;
}

void *room(void *id){
  int roomNumber = (int)id;
  int capacity = ROOM_CAPACITY;
  int roomState = 2;
  int numberOfStudents = 0;

  while(1){

    sem_wait(&remainingStudent);

    if(waitingStudents <= 0){ //IF THERE AREN'T ANY STUDENTS WAITING IN THE LIBRARY.
      sem_post(&remainingStudent);
      printf("No more students to study.Roomkeeper of room %d is going to clean the room.\n",roomNumber );
      break;
    }
    sem_post(&remainingStudent);
    sem_wait(rooms+roomNumber);
    roomState = roomStates[roomNumber];
    numberOfStudents = roomStudentNumber[roomNumber];
    sem_post(rooms+roomNumber);

    if (roomState == 2) { //IF ROOM STATE IS IN IDLE STATE
      printf("Roomkeeper of room %d is cleaning the room.\n",roomNumber );
    }
    else if(roomState == 1){ //IF ROOM IS IN ENTRY FREE STATE
      sem_wait(&remainingStudent);
      int q = waitingStudents;
      int flag = 0;
      if(q == numberOfStudents){
        flag = 1;
        printf("%d students are studying and 0 students are waiting in the library.\n",numberOfStudents);

        sem_wait(rooms+roomNumber);
        roomStates[roomNumber] = 3;
        sem_post(rooms+roomNumber);
      }
      sem_post(&remainingStudent);

      if(flag == 0){
        int emptySeats = capacity-numberOfStudents;
        printf("Room %d is not full. Roomkeeper calls: The last %d students, let's  get up!\n",roomNumber,emptySeats);
      }


      if(numberOfStudents == capacity){ //IF CAPACITY IS REACHED ROOM STATE= FULL AND BUSY STATE
        sem_wait(rooms+roomNumber);
        roomStates[roomNumber] = 3;
        sem_post(rooms+roomNumber);
      }
    }
    else if(roomState == 3){

      int studyTime = rand()%10+5;
      printf("Room %d is full and in the busy state.All students are studying.\n",roomNumber);

      sem_wait(&remainingStudent);
      waitingStudents = waitingStudents - numberOfStudents;
      sem_post(&remainingStudent);
      sem_wait(rooms+roomNumber);
      sem_post(rooms+roomNumber);
      sleep(studyTime);
      sem_wait(rooms+roomNumber);
      int b;

      for(b = 0;b<ROOM_CAPACITY;b++){ // ALL STUDENTS IN THE ROOM HAS STUDIED AND TIME TO EMPTY THE ROOM
        studyingStudents[roomNumber][b] = -1;
      }

      roomStates[roomNumber] = 2;
      roomStudentNumber[roomNumber] = 0;

      sem_post(rooms+roomNumber);

    }
    sleep(rand()%2+1);
  }

  return 0;
}


int main(){
    int i,j;

    for(i = 0 ;i<ROOM_NUMBER;i++){ //ROOM STATES IDLE AT THE BEGINNING
      roomStates[i] = 2;
    }

    for(i = 0 ;i<ROOM_NUMBER;i++){ //ROOMS ARE EMPTY AT THE BEGINNING
      roomStudentNumber[i] = 0;
    }

    for(i = 0;i<ROOM_NUMBER;i++){
      for(j = 0;j<ROOM_CAPACITY;j++){
        studyingStudents[i][j] = -1;
      }
    }
    //INITIAL VALUES
    for(i = 0;i<ROOM_NUMBER;i++){
      sem_init(rooms+i,0,1);
    }
    sem_init(&remainingStudent,0,1);

    int create;

    int room_id = 0;

    pthread_t roomThreads[ROOM_NUMBER];
    for(i=0;i<ROOM_NUMBER;i++) { // THREAD CREATION
       create = pthread_create(roomThreads+i, NULL, &room, (void*)room_id);
       room_id++;
    }

    int student_id = 0;

    pthread_t studentThreads[STUDENT_NUMBER];
    for(i=0;i<STUDENT_NUMBER;i++) { //THREAD CREATION
       create = pthread_create(studentThreads+i, NULL, &student, (void*)student_id);
       student_id++;
    }


    for(i=0;i<STUDENT_NUMBER;i++) { //THREAD JOIN
      int join = pthread_join(studentThreads[i], NULL);
    }


    for(i=0;i<ROOM_NUMBER;i++) { //THREAD JOIN
      int join = pthread_join(roomThreads[i], NULL);
    }


    printf("All of the students have studied.\n");
    return 0;
}
