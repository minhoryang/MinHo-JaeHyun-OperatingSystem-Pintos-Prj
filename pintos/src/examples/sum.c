#include <stdio.h>
#include <syscall.h>

int atoi(const char *in);

int
main (int argc, char **argv)
{
  int i, arr[4] = {0,};

  if(argc != 5)
  	  return -1;
  
  for(i=1; i<5; i++){
	  arr[i-1] = atoi(argv[i]);
  }

  printf ("%d %d\n",
		  pibonacci(arr[0]),
		  sum_of_four_integers(arr[0], arr[1], arr[2], arr[3]));

  return EXIT_SUCCESS;
}

int atoi(const char *in){
	size_t now = 0;
	int ret = 0;
	while(in[now]){
		ret *= 10;
		if(('0' <= in[now]) && (in[now] <= '9'))
			ret += in[now] - '0';
		now++;
	}
	
	return ret;
}
