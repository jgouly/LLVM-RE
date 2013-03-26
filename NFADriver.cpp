#include "NFA.h"
#include "TableCompiler.h"

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/Support/TargetSelect.h"

#include <iostream>

typedef int(*MatchFn)(const char *);

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

void test(MatchFn MFn, std::string s) {
  int m = match(MFn, s);
  if (m != -1)
    std::cout << "match at " << m << "\n";
  else
    std::cout << "no match" << "\n";
}

int main() {
  NFA END(true);
  NFA NC;
  NFA NB;
  NFA NA;
  NFA START;
  NC.addEdge('y', &END);
  NB.addEdge('e', &NC);
  NA.addEdge(NFA::Epsilon, &NB);
  NA.addEdge('o', &NB);
  START.addEdge('j', &NA);

  DFAGenerator DFAG(START);
  DFAG.gen();
  DFATable D = DFAG.get();
  TableCompiler C(D);
  C.gen();
  C.dump();

  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  std::string ErrStr;
  llvm::ExecutionEngine *EE =
      llvm::EngineBuilder(C.getModule()).setErrorStr(&ErrStr).setUseMCJIT(true)
      .setJITMemoryManager(new llvm::SectionMemoryManager()).create();
  if (!EE) {
    std::cerr << "Could not create ExecutionEngine: " << ErrStr.c_str() << "\n";
    return 1;
  }
  EE->finalizeObject();
  void *FPtr = EE->getPointerToFunction(C.getMatchFn());

  int(*match)(const char *) = (int(*)(const char *))(intptr_t) FPtr;
  test(match, "joey");
  test(match, "jey");
  test(match, "jay");
  return 0;
}
