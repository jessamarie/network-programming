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
int TEN = 10;
int TWO = 2;
unsigned int MAX = 4294967291;
unsigned int highest_num = 0;

/* is_prime checks if a number is prime.

@param num is the number to check
@return 0 if composite, or 1 if prime
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


/*
  find_primes finds the total number of primes from a given lo 
  to a given hi, with the help of a set number of cpus.
  
  @param lo the number to start from
  @param hi, the number to end at
  @param p_count the number of processes
  @param id the current rank
  @effect if a timeout occurs, set the current lo to the final number 
          achieved.
  @ return the total number of primes from lo to hi
*/

int find_primes(unsigned int *lo, unsigned int hi, int total_primes, int p_count, int id) {
  
  int primes = 0;
  int inc = p_count*2;
  unsigned int i;
  
  for (i = *lo; i <= hi; i = i+inc) {
    
    // check if prime
    primes += is_prime(i);
    
    if (end_now == 1) {
      MPI_Bcast ( &end_now, 1, MPI_INT, id, MPI_COMM_WORLD );

      *lo = i; // to get the highest number reached
      return total_primes + primes;
    }
    
  }
  
  return total_primes + primes;
  
}

/* is_power checks if a number num is a power
    of y.
    
    @param num, the number to check
    @param y, the power
    @return 0 if false, 1 if true.

*/
int is_power(unsigned int num, int y) {

  while (num % y == 0) {
    num /= 10;
  }

  if (num == 1) return 1;

  return 0;
}

/* sig_handler is the signal 
    handler

*/
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
    
    unsigned int curr_hi = TEN;
    unsigned int curr_lo = (id * 2) + 1;


    while (1) {
      
     // MPI_Bcast ( &curr_lo, 1, MPI_INT, 0, MPI_COMM_WORLD );
      
      current_primes = find_primes(&curr_lo, curr_hi, current_primes, count, id);
      
      // gets the sum of all primes found across processes
      MPI_Reduce ( &current_primes, &primes, 1, MPI_INT, MPI_SUM, 0,
          MPI_COMM_WORLD );
          
      // gets the maximum number reached so far
      MPI_Reduce ( &curr_lo, &highest_num, 1, MPI_INT, MPI_MAX, 0, 
          MPI_COMM_WORLD );
      
      if (end_now == 1) {
        
        // print out the most recent prime
        if (id == 0) {
          printf("%8d %8d\n", highest_num, primes);
        }
        break;
      } else if ( id == 0) {
        // print out current primes at every power of 10
          printf ( "%8d  %8d\n", curr_hi, primes);
      }
        
      if (curr_lo == MAX) {
        printf("%8d  %8d\n", MAX, primes);
        break;
      }
      
      // move up the current low and multiply hi by a factor of 10
      curr_lo = (id * 2) + curr_hi + 1;
      if (curr_hi * TEN < MAX) {
        curr_hi = curr_hi * TEN;
      } else {
        curr_hi = MAX;
      }
      
    }
    
    // Finished
    MPI_Finalize();

    return 0;
}
;
