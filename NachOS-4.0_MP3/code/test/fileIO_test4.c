#include "syscall.h"

int main(void)
{
	// you should run fileIO_test3 first before running this one
	char test[25][26];
	char check[] = "abcdefghijklmnopqrstuvwxyz";
	char myname[] = "filex.test";
	OpenFileId fid[25];
	int count, success, i, j;

	// Read an un-opened file
	success = Read(test[0], 1, 0);
	if (success >= 0) MSG("Read an un-opened file");

	// Try to open and read 22 files
    	for (i = 0; i < 22; i++) {
		myname[4] = check[i];
        	fid[i] = Open(myname);
        	if (fid[i] < 0) {
            		if (i < 20) MSG("Failed on opening file");
			else {
				MSG("RIGHT: Cannot open more than 20 files");
			}
        	}
        	count = Read(test[i], 26, fid[i]);
        	if (count != 26) {
            		if(i < 20) MSG("Failed on reading file");
			else {
				MSG("RIGHT: Cannot read an un-opened file");
			}
        	}
    	}	

    	// Try to close 22 files
    	for (i = 0; i < 22; i++) {
        	success = Close(fid[i]);
        	if (success != 1) {
            		if (i < 20) MSG("Failed on closing file");
            		else {
                		MSG("RIGHT: Cannot close an un-opened file");
            		}
        	}
        	for (j = 0; j < 26; ++j) {
            		if (test[i][j] != check[(i + j) % 26]) {
                		if (i < 20) MSG("Failed: reading wrong result");
                		else {
                    			MSG("RIGHT: Did not read from an un-opened file");
                		}
            		}
        	}
	}

	MSG("Passed! ^_^");
	Halt();
}


