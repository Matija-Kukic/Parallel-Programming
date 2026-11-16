.PHONY: clean

CPP_FILES := arraymalloc.cpp boundary.cpp cfd.cu cfdio.cpp jacobi.cu
INC := -I/home/mlkukic/Projects/parpro/lab3/3.3/include

cfd: $(CPP_FILES) 
	$(NVCC) -ccbin $(CCBIN) $(CXXFLAGS) $(INC) $(CPP_FILES) -o cfd $(LDFLAGS)


clean:
	rm cfd
