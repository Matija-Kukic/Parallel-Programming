CC = mpic++

n ?= 7
d ?= 6
a ?= 1
.PHONY: run src

serun: 
	cp start.txt ploca.txt
	./lab2 ploca.txt $d
run: 
	cp start.txt ploca.txt
	mpiexec --oversubscribe -n $(n) ./lab2_par ploca.txt $d $a

serial: main_ser.cpp board.cpp
	$(CC) -I/usr/lib64/mpi/gcc/openmpi5/include $^ -o lab2
# paralel: 
par: main_par.cpp board.cpp
	$(CC) -I/usr/lib64/mpi/gcc/openmpi5/include $^ -o lab2_par

src:
	source mpivars.sh


