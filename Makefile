# ===== Compilers =====
CXX        := g++
NVCC	   := nvcc

# ===== Flags =====
CXXFLAGS   := -O3 -std=c++17
NVCCFLAGS  := -O3 -arch=sm_86 -Xcompiler -fopenmp

# ===== Targets =====
TARGET_CPP := edgelist2ecl
TARGET_CU  := mis

# ===== Srcs =====
SRCS_CPP   := edgelist2ecl.cpp
SRCS_CU    := MG-MIS_10.cu
HDRS       := ECLgraph.h

# ===== Build All =====
all: $(TARGET_CPP) $(TARGET_CU)

# ===== C++ build =====
$(TARGET_CPP): $(SRCS_CPP) $(HDRS)
	$(CXX) $(CXXFLAGS) $(SRCS_CPP) -o $(TARGET_CPP)

# ===== CUDA build =====
$(TARGET_CU): $(SRCS_CU)
	$(NVCC) $(NVCCFLAGS) $(SRCS_CU) -o $(TARGET_CU)

# ===== Clean =====
clean:
	rm -f $(TARGET_CPP) $(TARGET_CU) *.o

.PHONY: all clean


