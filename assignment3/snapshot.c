/* Author: Jessica Marie Barre
   Name: Prime Finder
   Purpose: A prime number finding program that utilizes
            multiple cores via MPI.
   Due: Sunday, April 16
*/

/***** Goals ****

- Program should return the number of unique primes
  within a time limit
- The sizeable amounts of computation make this problem
a good candidate for parallelizing via MPI.
- Fortunately, there is very little coordination required among the different ranks although the primes should be printed out in order.
- Additionally, should you exhaust all
(unsigned) 32-bit primes, your program can stop.
- Otherwise, a signal will be sent to your program after a
fixed amount of time (likely 1-2 minutes).
-  Also please include a speedup graph as the number of ranks is increased and explain any unexpected results. Hopefully everyone can run their program on 1, 2, and 4 cores at least

- USE OPEN MPI

So we don’t anger the Submitty gods, instead of saving gigabytes of integers, we can instead
use the number of primes less than a specific value as the following output demonstrates:

Sample output given below:

timeout -s 10 5 mpirun -np 2 ./find-primes

bash$ mpirun-openmpi-mp -np 2 ./a.out
N                   Primes
10                    4
100                   25
1000                  168
<Signal received>
1020                171

*/

#include <mpi.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <signal.h>

int end_now = 0;
int START = 10;
int TEN = 10;
int MAX_= 2147483647;

/* get_primes returns the number of primes
between 1 and num.

num is the highest number
id is the id of the process
p_count is the number of processes

returns the number of primes between 1 and N
*/
int get_primes(int num, int id, int p_count) {

  int i, j, totalPrimes;

  for (i = 2 + id; i <= num; i = i + p_count) {
    int prime = 1;

    for ( j = 2; j < i; j++) {
      if ( (i % j) == 0) {
        prime = 0;

        // since we found at least 1 number that divides i
        // we know it is not a prime
        // So we can skip outside this loop
        break;
      }
    }

    totalPrimes = totalPrimes + prime;
  }
  return totalPrimes;
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
    int max_num = START;
    int num_primes;
    int primes;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &count);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);

    printf("%8s  %8s\n", "N", "primes");

    signal(SIGUSR1, sig_handler);

    while (1) {

      MPI_Bcast ( &max_num, 1, MPI_INT, 0, MPI_COMM_WORLD );

      num_primes = get_primes(max_num, id, count);

      printf("%d\n", num_primes);

      MPI_Reduce(&num_primes, &primes, 1, MPI_INT, MPI_SUM, 0,
        MPI_COMM_WORLD );

        printf("%d\n", num_primes);
        printf("%d\n", primes);

      if ( id == 0 ) {
        printf("%8d  %8d\n", max_num, primes);
      }

      if (end_now == 1) {
          break;
      }

        max_num = max_num * TEN;
    }

    MPI_Finalize();

    return 0;
}
