CC = mpic++

n ?= 4
.PHONY: run src

run: lab1
	mpiexec -n $(n) ./lab1

lab1: lab1.cpp
	$(CC) -I/usr/lib64/mpi/gcc/openmpi5/include lab1.cpp -o lab1 

src:
	source mpivars.sh


