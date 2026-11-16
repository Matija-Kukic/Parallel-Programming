NVCC := nvcc
CCBIN := g++-15

CXXFLAGS := -std=c++17 -O2
LDFLAGS := -lstdc++

TARGET := vec_add
SRC := vec_add.cu

scale ?= 64
numiter ?= 1000

export NVCC CCBIN CXXFLAGS LDFLAGS

.PHONY: test 3.1 3.2 clean
test: vec_add
	./vec_add

$(TARGET):
	$(NVCC) -ccbin $(CCBIN) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

clean:
	- rm -f $(TARGET)
	- make -C 3.1 clean
	- make -C 3.2 clean
	- make -C 3.3 clean

3.1: 3.1/prime_cnt
	./3.1/prime_cnt

3.1/prime_cnt: 
	make -C 3.1

3.2: 3.2/pi_sum
	./3.2/pi_sum

3.2/pi_sum:
	make -C 3.2 pi_sum

3.3: 3.3/cfd
	./3.3/cfd $(scale) $(numiter)
3.3/cfd:
	make -C 3.3 cfd
