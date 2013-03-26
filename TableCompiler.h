#include "DFATable.h"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>

class TableCompiler {
public:
  TableCompiler(DFATable &D)
      : D(D), C(&llvm::getGlobalContext()), M(new llvm::Module("llvm-re", *C)),
        StateBlocks(), B(llvm::IRBuilder<>(*C)), UniqueChars() {
  }

  llvm::Module *getModule() const { return M; }
  llvm::Function *getMatchFn() const { return F; }
  void gen();
  void dump();

private:
  inline llvm::ConstantInt *getInt(int N);
  llvm::Value *getCurChar(llvm::IRBuilder<> B);
  void generateStateMatch(unsigned I);
  void genCharIndexFn();
  void genFinalTable();
  void genTransitionTable();
  unsigned getCharIndex(char C);
  DFATable &D;
  llvm::LLVMContext *C;
  llvm::Module *M;
  llvm::Function *F, *CharIndexFn;
  std::vector<llvm::BasicBlock *> StateBlocks;
  llvm::IRBuilder<> B;
  std::set<char> UniqueChars;
  llvm::Value *String, *Index;
  llvm::Value *IsMatch;
  llvm::GlobalVariable *Table, *FinalTable;
  llvm::Value *CurState;
};
