/* Author: Jessica Marie Barre
   Name: Prime Finder
   Purpose: A prime number finding program that utilizes
            multiple cores via MPI.
   Due: Sunday, April 16
*/

/***** Goals ****

- USE OPEN MPI
- Return # of Unique primes w/in timelimit in format:
N                   Primes
10                    4
100                   25
1000                  168
<Signal received>
1020                171

- Parallelize
- Should you exhaust all (unsigned) 32-bit primes, your program can stop.
- Otherwise, a signal will be sent to your program after a
fixed amount of time (likely 1-2 minutes).
-  Also please include a speedup graph
showing ranks vs # of primes found.  explain any unexpected results.

Commit with:
mpicc -g -Wall -o mpi_primes barrej4_hw3.c
Run with:
timeout -s 10 5 mpirun -np 1 ./mpi_primes


*/

#include <mpi.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <signal.h>
#include <unistd.h>


int end_now = 0;
int START = 1;
int TEN = 10;
int TWO = 2;
unsigned int MAX = 4294967291;

/* get_primes returns the number of primes
between 1 and num.

num is the highest number
id is the id of the process
p_count is the number of processes

returns the number of primes between 1 and N
*/

int is_prime(unsigned int num) {

  unsigned int j;
  int prime = 1;

  for ( j = 2; j < num; j++) {
    if ( (num % j) == 0) {
      prime = 0;
      // since we found at least 1 number that divides i
      // we know it is not a prime
      // So we can skip outside this loop
      break;
    }
  }
    return prime;
}

int find_primes(unsigned int lo, unsigned int hi, int p_count, int id) {
  
  
}

int is_power(unsigned int num, int y) {

  while (num % y == 0) {
    num /= 10;
  }

  if (num == 1) return 1;

  return 0;
}

void sig_handler(int signo)
{
    if (signo == SIGUSR1) {
        end_now = 1;
    }
}

int main(int argc, char **argv)
{
    int count, id;
    int primes = 0;
    int current_primes = 0;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &count);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);

    if (id == 0) {
      printf("%8s  %8s\n", "N", "primes");
    }

    signal(SIGUSR1, sig_handler);

    while (1) {

      unsigned int i;

      for (i = id + TWO; i <= MAX; i = i+count) {

        MPI_Bcast ( &MAX, 1, MPI_INT, 0, MPI_COMM_WORLD );
        
        // check if prime
        current_primes += is_prime(i);

        if (i <= 10) {
        //  printf("process %d with i=%d and primes %d\n", id, i, current_primes);
        }

        if (is_power(i, TEN)) {
          printf ( "%8d  %8d\n", i, primes);
        }

        MPI_Reduce ( &current_primes, &primes, 1, MPI_INT, MPI_SUM, 0,
          MPI_COMM_WORLD );

        if (end_now == 1) {
          if(id == 0) {
            printf("%8d  %8d\n", i, primes);
          }
          break;
        }

      }

      if (end_now == 1) {
        break;
      }

    }

    MPI_Finalize();

    return 0;
}
;
