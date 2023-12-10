make clean
make
../build.linux/nachos -f
../build.linux/nachos -cp num_100.txt /f1
../build.linux/nachos -cp num_1000.txt /f2
../build.linux/nachos -cp num_1000000.txt /f3
../build.linux/nachos -lr /
../build.linux/nachos -D /