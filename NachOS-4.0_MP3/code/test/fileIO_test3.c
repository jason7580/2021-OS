#include "syscall.h"

int main(void)
{
	char test[] = "abcdefghijklmnopqrstuvwxyz";
	char myname[] = "filex.test";
	int success;
	OpenFileId fid[25];

	int i, j;
	
	// Open a non-exist file
 	success = Open("aaaaaaaaaaa");
	if (success >= 0) MSG("Open a non-exist file");

	// Write on an un-opened file
	success = Write(test, 1, 0);
	if (success >= 0) MSG("Write on an un-opened file");

	// Close an un-opened file
	success = Close(0);
	if (success >= 0) MSG("Close an un-opened file 0");
	success = Close(-1);
	if (success >= 0) MSG("Close an un-opened file -1");

	// Try to create and write 22 files
	for (i = 0; i < 22; i++) {
		myname[4] = test[i];
		success = Create(myname);
		
		if (success != 1) MSG("Failed on creating file");
		fid[i] = Open(myname);
		
		if (fid[i] < 0) {
			if (i < 20) MSG("Failed on opening file");
			else {
				MSG("RIGHT: Cannot open more than 20 files");
			}
		}
		
		for (j = 0; j < 26; ++j) {
			int count = Write(test + (i + j) % 26, 1, fid[i]);
			if (count != 1) {
                		if (i < 20) MSG("Failed on writing file");
                		else {
                    			MSG("RIGHT: Cannot write on an un-opened file");
                		}
            		}
		}
	}

	// Try to close 22 files
	for(i = 0; i < 22; i++){
        	success =  Close(fid[i]);
        	if (success != 1) {
            		if (i < 20) MSG("Failed on closing file");
            		else {
            			MSG("RIGHT: Cannot close an un-opened file");
            		}
        	}
	}
	
	MSG("Success on creating file3.test");
	Halt();
}
