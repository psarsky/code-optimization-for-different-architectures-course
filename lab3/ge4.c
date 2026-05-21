#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

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

int ge(double **A, int SIZE)
{
  register int i, j, k;
  for (k = 0; k < SIZE; k++) {
    for (i = k+1; i < SIZE; i++) {
      register double multiplier = (A[i][k] / A[k][k]);
      for (j = k+1; j < SIZE; ) {
        if (j < MAX(SIZE - BLKSIZE, 0)) {
          A[i][j]   = A[i][j]   - A[k][j]   * multiplier;
          A[i][j+1] = A[i][j+1] - A[k][j+1] * multiplier;
          A[i][j+2] = A[i][j+2] - A[k][j+2] * multiplier;
          A[i][j+3] = A[i][j+3] - A[k][j+3] * multiplier;
          A[i][j+4] = A[i][j+4] - A[k][j+4] * multiplier;
          A[i][j+5] = A[i][j+5] - A[k][j+5] * multiplier;
          A[i][j+6] = A[i][j+6] - A[k][j+6] * multiplier;
          A[i][j+7] = A[i][j+7] - A[k][j+7] * multiplier;
          j += BLKSIZE;
        }
        else {
          A[i][j] = A[i][j] - A[k][j] * multiplier;
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

  double *data = (double *)malloc((size_t)SIZE * SIZE * sizeof(double));
  double **matrix = (double **)malloc(SIZE * sizeof(double *));
  for (i = 0; i < SIZE; i++)
    matrix[i] = data + i * SIZE;

  srand(1);
  for (i = 0; i < SIZE; i++) {
    for (j = 0; j < SIZE; j++) {
      matrix[i][j] = rand();
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
      check = check + matrix[i][j];
    }
  }
  printf("Check: %le \n", check);
  fflush(stdout);

  free(data);
  free(matrix);

  return iret;
}
