#include "syscall.h"

int
main()
{
	int n, i;
  int array[1000];
  /*for(i=0;i<1000;i++){
    array[i]=i;
  }*/
	for (n=30;n<100;n++) {
		PrintInt(n);
	}
	
	return 0;
        //Halt();
}
