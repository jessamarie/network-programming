#include <stdio.h>
#include <string.h>
#include <mpi.h>

/* Compile + execute instructions
mpicc -g -Wall -o mpi_hello filename.c

mpiexec -n <number of processes> <executable>
mpiexec -n 4 ./mpi_hello

*/

const int MAX_STRING = 100;

int main(void) {

  char greeting[MAX_STRING];
  int comm_sz; /* Number of processes */
  int my_rank; /* process rank */

  // Tells MPI to do all the necessary setup.
  // (int argc_p, char*** argv_P)
  MPI_Init(NULL, NULL);


  /* Communicators are a collection of
  processes that can send messages to each other

  MPI_Init defines a communicator that
  consists of all the processes created when
  the program is started.

  CALLED MPI_COMM_WORLD
  */

  // comm_sz is the number of processors in the communicator
  MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
  // my_rank_p is the process making the call
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

  if (my_rank !=0) {
    sprintf(greeting, "Greetings from process %d of %d!",
      my_rank, comm_sz);

    // msg buffer, size, MPI_type, dest, tag, communicator
    MPI_Send(greeting, strlen(greeting)+1, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
  } else {
    printf("Greetings from process %d of %d!\n", my_rank, comm_sz);

    for (int q = 1; q < comm_sz; q++) {

      /* A receiver can get a message without knowing:
        - the amt of data in the message,
        - the sender of the message,
        - or the tag of the message.
      */
      // msg buffer, buf_size, buf_type, source, tag, communicator, status_p
      MPI_Recv(greeting, MAX_STRING, MPI_CHAR, q, 0,
        MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      printf("%s\n", greeting);
    }
  }


  /** Other MPI functions
  int MPI_Get_count(statys_p, type, count_p) //how much data am I recieving?
  **/

  // Tells MPI we are done!
  // Cleans up all allocations
  MPI_Finalize();
  return 0;
}
