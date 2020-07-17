all: 
	nvcc -w -std=c++11 ri3.cu -I ./rilib -I ./include/ -o ri36