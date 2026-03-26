//requires additional changes to the code to make it work

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <string>
#include <iostream>

#define REPEATS 50000

static double gtod_ref_time_sec = 0.0;

/* Adapted from the bl2_clock() routine in the BLIS library */

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



std::string remove_ctrl(std::string s){
    std::string result;
    result.reserve(s.length());
    for (int i = 0; i < s.length(); i++){
        if (s[i] >= 0x20)
            result += s[i];
    }
    return result;
}


int main( int argc, const char* argv[] )
{
  int i,j,k,iret;
  double dtime;
  
  std::string s;
  std::string result;
  
  std::string line;
  while (getline(std::cin, line)){
      s += line + "\n";
  }
  
  dtime = dclock();
  for (int i = 0; i < REPEATS; i++){
      result = remove_ctrl(s);
  }
  dtime = dclock() - dtime;
  
  std::cout << result << "\n";  
  std::cout << "Time: " << dtime << "\n";
  fflush( stdout );

  return iret;
}

