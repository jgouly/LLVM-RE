CXX=clang++
LLVMCONFIG=/home/jey/code/llvm/build-debug-ninja/bin/llvm-config
CXXFLAGS=-std=c++11 -g

all: Driver.o TableCompiler.o
	$(CXX) $(CXXFLAGS) $^ `$(LLVMCONFIG) --cxxflags --ldflags --libs all`

nall: NFADriver.o TableCompiler.o
	$(CXX) $(CXXFLAGS) $^ `$(LLVMCONFIG) --cxxflags --ldflags --libs all`

Driver.o: Driver.cpp
	$(CXX) $(CXXFLAGS) -c $^ `$(LLVMCONFIG) --cxxflags`

TableCompiler.o: TableCompiler.cpp
	$(CXX) $(CXXFLAGS) -c $^ `$(LLVMCONFIG) --cxxflags`

NFADriver.o: NFADriver.cpp
	$(CXX) $(CXXFLAGS) -c $^ `$(LLVMCONFIG) --cxxflags`

clean:
	rm *.o
