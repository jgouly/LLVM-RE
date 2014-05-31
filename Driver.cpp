#include "TableCompiler.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/Support/TargetSelect.h"

#include <iostream>

typedef int (*MatchFn)(const char *);

int match(MatchFn MFn, std::string Str) {
  unsigned Index = 0;
  const char *str = Str.c_str();
  unsigned Len = Str.size();
  while (Index < Len) {
    if (MFn(str + Index))
      return Index;
    ++Index;
  }
  return -1;
}

int main(int argc, char **) {
  // regex = /ab/
  // DFATable D { { { {'b', 1} }, { {'b', 2} }, { {'c', 3}, {'q', 3} } } };
  DFATable D{{{{'a', 1}},
              {{'b', 1}, {'c', 2}},
              {{'c', 2 | DFATable::Final}, {'d', 3}},
              {{'f', 4}}}};

  TableCompiler C(D);
  C.gen();
  C.dump();
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  std::string ErrStr;
  llvm::ExecutionEngine *EE = llvm::EngineBuilder(C.getModule())
                                  .setErrorStr(&ErrStr)
                                  .setUseMCJIT(true)
                                  .create();
  if (!EE) {
    fprintf(stderr, "Could not create ExecutionEngine: %s\n", ErrStr.c_str());
    return 1;
  }
  EE->finalizeObject();
  void *FPtr = EE->getPointerToFunction(C.getMatchFn());

  int (*match_ab)(const char *) = (MatchFn) FPtr;
  if (argc > 1) {
    int m = match(match_ab, "abcc");
    if (m != -1) {
      std::cout << "MATCH at " << m;
    }
  }
  return 0;
}
