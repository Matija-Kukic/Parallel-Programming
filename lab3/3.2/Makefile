.PHONY: clean 
pi_sum: pi_sum.cu
	$(NVCC) -ccbin $(CCBIN) $(CXXFLAGS) pi_sum.cu -o pi_sum $(LDFLAGS)
clean:
	rm pi_sum
