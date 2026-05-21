#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <x86intrin.h>

#define IDX(i, j, SIZE) ((i) * (SIZE) + (j))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define MR 8
#define NR 8
#define MC 128
#define NC 256

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

static void PackPivotRow(int n, const double *src, double *dst)
{
  int j;
  for (j = 0; j < n; j++) {
    dst[j] = src[j];
  }
}

static void PackMultipliers(int m, const double *A, int SIZE,
                            int row0, int k, double pivot,
                            double *mult)
{
  int i;
  for (i = 0; i < m; i++) {
    mult[i] = A[IDX(row0 + i, k, SIZE)] / pivot;
  }
}

static void UpdateKernel(int m, int n, double *C,
                         int ldc, const double *pivot_row,
                         const double *mult)
{
  int i, j;

  for (j = 0; j + NR <= n; j += NR) {
    register __m128d p01 = _mm_loadu_pd(pivot_row + j);
    register __m128d p23 = _mm_loadu_pd(pivot_row + j + 2);
    register __m128d p45 = _mm_loadu_pd(pivot_row + j + 4);
    register __m128d p67 = _mm_loadu_pd(pivot_row + j + 6);

    for (i = 0; i < m; i++) {
      double *c = C + i * ldc + j;

      register __m128d mmult = _mm_set1_pd(mult[i]);

      register __m128d c01 = _mm_loadu_pd(c);
      register __m128d c23 = _mm_loadu_pd(c + 2);
      register __m128d c45 = _mm_loadu_pd(c + 4);
      register __m128d c67 = _mm_loadu_pd(c + 6);

      c01 = _mm_sub_pd(c01, _mm_mul_pd(p01, mmult));
      c23 = _mm_sub_pd(c23, _mm_mul_pd(p23, mmult));
      c45 = _mm_sub_pd(c45, _mm_mul_pd(p45, mmult));
      c67 = _mm_sub_pd(c67, _mm_mul_pd(p67, mmult));

      _mm_storeu_pd(c,     c01);
      _mm_storeu_pd(c + 2, c23);
      _mm_storeu_pd(c + 4, c45);
      _mm_storeu_pd(c + 6, c67);
    }
  }

  for (; j < n; j++) {
    register double pj = pivot_row[j];
    for (i = 0; i < m; i++) {
      C[i * ldc + j] = C[i * ldc + j] - pj * mult[i];
    }
  }
}

int ge(double *A, int SIZE)
{
  int k;
  int ii, jj;

  double packed_pivot[NC];
  double packed_mult[MC];

  for (k = 0; k < SIZE; k++) {
    double pivot = A[IDX(k, k, SIZE)];

    for (jj = k + 1; jj < SIZE; jj += NC) {
      int jb = MIN(NC, SIZE - jj);

      PackPivotRow(jb, &A[IDX(k, jj, SIZE)], packed_pivot);

      for (ii = k + 1; ii < SIZE; ii += MC) {
        int ib = MIN(MC, SIZE - ii);

        PackMultipliers(ib, A, SIZE, ii, k, pivot, packed_mult);

        UpdateKernel(ib, jb, &A[IDX(ii, jj, SIZE)],
                     SIZE, packed_pivot, packed_mult);
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
