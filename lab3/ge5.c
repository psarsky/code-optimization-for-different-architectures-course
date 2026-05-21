#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#define IDX(i, j, SIZE) ((i) * (SIZE) + (j))
#define BLKSIZE 8
#define MAX(a, b) ((a) > (b) ? (a) : (b))

static double gtod_ref_time_sec = 0.0;

double ge_flop(int SIZE)
{
    double n = (double) SIZE;
    return (n - 1.0) * n * (2.0 * n - 1.0) / 2.0;
}

double dclock()
{
  double the_time, norm_sec;
  struct timeval tv;
  gettimeofday( &tv, NULL );
  if ( gtod_ref_time_sec == 0.0 )
    gtod_ref_time_sec = ( double ) tv.tv_sec;
  norm_sec = ( double ) tv.tv_sec - gtod_ref_time_sec;
  the_time = norm_sec + tv.tv_usec * 1.0e-6;
  return the_time;
}

int ge(double *A, int SIZE)
{
  register int i, j, k;
  for (k = 0; k < SIZE; k++) {
    for (i = k+1; i < SIZE; i++) {
      register double multiplier = (A[IDX(i,k,SIZE)] / A[IDX(k,k,SIZE)]);
      for (j = k+1; j < SIZE; ) {
        if (j < MAX(SIZE - BLKSIZE, 0)) {
          A[IDX(i,j,SIZE)]   = A[IDX(i,j,SIZE)]   - A[IDX(k,j,SIZE)]   * multiplier;
          A[IDX(i,j+1,SIZE)] = A[IDX(i,j+1,SIZE)] - A[IDX(k,j+1,SIZE)] * multiplier;
          A[IDX(i,j+2,SIZE)] = A[IDX(i,j+2,SIZE)] - A[IDX(k,j+2,SIZE)] * multiplier;
          A[IDX(i,j+3,SIZE)] = A[IDX(i,j+3,SIZE)] - A[IDX(k,j+3,SIZE)] * multiplier;
          A[IDX(i,j+4,SIZE)] = A[IDX(i,j+4,SIZE)] - A[IDX(k,j+4,SIZE)] * multiplier;
          A[IDX(i,j+5,SIZE)] = A[IDX(i,j+5,SIZE)] - A[IDX(k,j+5,SIZE)] * multiplier;
          A[IDX(i,j+6,SIZE)] = A[IDX(i,j+6,SIZE)] - A[IDX(k,j+6,SIZE)] * multiplier;
          A[IDX(i,j+7,SIZE)] = A[IDX(i,j+7,SIZE)] - A[IDX(k,j+7,SIZE)] * multiplier;
          j += BLKSIZE;
        }
        else {
          A[IDX(i,j,SIZE)] = A[IDX(i,j,SIZE)] - A[IDX(k,j,SIZE)] * multiplier;
          j++;
        }
      }
    }
  }
  return 0;
}

int main( int argc, const char* argv[] )
{
  int i, j, k, iret;
  double dtime;
  int SIZE = atoi(argv[1]);

  double *matrix = (double *)malloc((size_t)SIZE * SIZE * sizeof(double));

  srand(1);
  for (i = 0; i < SIZE; i++) {
    for (j = 0; j < SIZE; j++) {
      matrix[IDX(i,j,SIZE)] = rand();
    }
  }
  printf("call GE");
  dtime = dclock();
  iret = ge(matrix, SIZE);
  dtime = dclock() - dtime;
  printf("Time: %le \n", dtime);
  printf("FLOP: %le \n", ge_flop(SIZE));
  printf("GFLOP/s: %le \n", ge_flop(SIZE) / dtime / 1.0e9);

  double check = 0.0;
  for (i = 0; i < SIZE; i++) {
    for (j = 0; j < SIZE; j++) {
      check = check + matrix[IDX(i,j,SIZE)];
    }
  }
  printf("Check: %le \n", check);
  fflush(stdout);

  free(matrix);

  return iret;
}
