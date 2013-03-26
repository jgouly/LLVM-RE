#include "TableCompiler.h"
#include <llvm/Analysis/Verifier.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>

static llvm::Function *generateMatchFunction(llvm::Module *M) {
  llvm::Type *T[] = { llvm::PointerType::getUnqual(
      llvm::Type::getInt8Ty(M->getContext())) };
  llvm::FunctionType *FT = llvm::FunctionType::get(
      llvm::Type::getInt32Ty(M->getContext()), T, false);
  return llvm::Function::Create(FT, llvm::Function::ExternalLinkage, "match",
                                M);
}

llvm::Value *TableCompiler::getCurChar(llvm::IRBuilder<> B) {
  llvm::Value *Idx = { B.CreateLoad(Index) };
  return B.CreateLoad(B.CreateGEP(String, Idx, "CurChar"));
}

void TableCompiler::genCharIndexFn() {
  llvm::Type *T[] = { B.getInt8Ty() };
  llvm::FunctionType *FT = llvm::FunctionType::get(B.getInt32Ty(), T, false);
  CharIndexFn = llvm::Function::Create(FT, llvm::Function::InternalLinkage,
                                       "getcharindex", M);
  llvm::BasicBlock *Entry = llvm::BasicBlock::Create(*C, "entry", CharIndexFn);
  llvm::BasicBlock *Default =
      llvm::BasicBlock::Create(*C, "default", CharIndexFn);
  llvm::IRBuilder<> B(Default);
  B.CreateRet(B.getInt32(-1));
  B.SetInsertPoint(Entry);
  llvm::SwitchInst *Switch =
      B.CreateSwitch(CharIndexFn->arg_begin(), Default, UniqueChars.size());
  std::vector<llvm::BasicBlock *> Rets;
  Rets.reserve(UniqueChars.size());
  unsigned Idx = 0;
  for (auto I = UniqueChars.cbegin(); I != UniqueChars.cend(); ++I, ++Idx) {
    llvm::BasicBlock *BB = llvm::BasicBlock::Create(*C, "", CharIndexFn);
    Switch->addCase(B.getInt8(*I), BB);
    llvm::IRBuilder<> B2(BB);
    B2.CreateRet(B.getInt32(Idx));
  }
}

void TableCompiler::genTransitionTable() {
  unsigned OuterSize = D.States.size();
  llvm::ArrayType *InnerAT =
      llvm::ArrayType::get(B.getInt32Ty(), UniqueChars.size());
  llvm::ArrayType *OuterAT = llvm::ArrayType::get(InnerAT, OuterSize);
  std::vector<llvm::Constant *> OuterElts;
  for (auto &Row : D.States) {
    std::vector<llvm::Constant *> Elts;
    for (char C : UniqueChars) {
      if (Row.find(C) != Row.end())
        Elts.push_back(B.getInt32(Row.find(C)->second & ~DFATable::Final));
      else
        Elts.push_back(B.getInt32(-1));
    }
    llvm::Constant *InnerInit = llvm::ConstantArray::get(InnerAT, Elts);
    OuterElts.push_back(InnerInit);
  }
  llvm::Constant *OuterInit = llvm::ConstantArray::get(OuterAT, OuterElts);
  Table = new llvm::GlobalVariable(*M, OuterAT, true,
                                   llvm::GlobalVariable::InternalLinkage,
                                   OuterInit, "transitiontable");
}

void TableCompiler::genFinalTable() {
  llvm::ArrayType *AT = llvm::ArrayType::get(B.getInt32Ty(), D.States.size());
  std::vector<llvm::Constant *> Finals;
  Finals.reserve(D.States.size());
  for (auto &Row : D.States) {
    bool isFinal = false;
    for (auto &Cell : Row) {
      if ((DFATable::Final & Cell.second) != 0)
        isFinal = true;
    }
    Finals.push_back(B.getInt32(isFinal));
  }
  llvm::Constant *Init = llvm::ConstantArray::get(AT, Finals);
  Init->dump();
  FinalTable = new llvm::GlobalVariable(
      *M, AT, true, llvm::GlobalVariable::InternalLinkage, Init, "FinalTable");
}

void TableCompiler::gen() {
  F = generateMatchFunction(M);
  UniqueChars = D.getUniqueChars();
  genTransitionTable();
  genFinalTable();
  llvm::BasicBlock *Entry = llvm::BasicBlock::Create(*C, "entry", F);
  llvm::BasicBlock *Fail = llvm::BasicBlock::Create(*C, "fail", F);
  llvm::BasicBlock *Success = llvm::BasicBlock::Create(*C, "success", F);
  genCharIndexFn();

  llvm::IRBuilder<> B(Fail);
  B.CreateRet(B.getInt32(0));
  B.SetInsertPoint(Success);
  B.CreateRet(B.getInt32(1));
  B.SetInsertPoint(Entry);
  Index = B.CreateAlloca(B.getInt32Ty(), nullptr, "Index");
  B.CreateStore(B.getInt32(0), Index);
  IsMatch = B.CreateAlloca(B.getInt32Ty(), nullptr, "IsMatch");
  B.CreateStore(B.getInt32(0), IsMatch);
  CurState = B.CreateAlloca(B.getInt32Ty(), nullptr, "CurState");
  B.CreateStore(B.getInt32(0), CurState);

  B.SetInsertPoint(Entry);

  String = F->arg_begin();

  llvm::BasicBlock *LoopStart = llvm::BasicBlock::Create(*C, "loop.start", F);
  llvm::BasicBlock *LoopCont = llvm::BasicBlock::Create(*C, "loop.cont", F);
  llvm::BasicBlock *LoopEnd = llvm::BasicBlock::Create(*C, "loop.end", F);
  B.CreateBr(LoopStart);

  B.SetInsertPoint(LoopStart);
  llvm::Value *LoadedChar = getCurChar(B);
  llvm::Value *CurCharIndex = B.CreateCall(CharIndexFn, LoadedChar);
  llvm::Value *CondChar = B.CreateICmpEQ(CurCharIndex, B.getInt32(-1));
  B.CreateCondBr(CondChar, Fail, LoopCont);

  B.SetInsertPoint(LoopCont);
  llvm::Value *Indices[] = { B.getInt32(0), B.CreateLoad(CurState),
                             CurCharIndex };
  llvm::Value *NextState = B.CreateLoad(B.CreateGEP(Table, Indices));
  llvm::Value *Cond = B.CreateICmpEQ(NextState, B.getInt32(-1));
  B.CreateCondBr(Cond, Fail, LoopEnd);

  B.SetInsertPoint(LoopEnd);
  B.CreateStore(NextState, CurState);
  B.CreateStore(B.CreateAdd(B.CreateLoad(Index), B.getInt32(1)), Index);
  llvm::Value *FinalIndices[] = { B.getInt32(0), B.CreateLoad(CurState) };
  llvm::Value *IsFinal = B.CreateICmpEQ(
      B.getInt32(1), B.CreateLoad(B.CreateGEP(FinalTable, FinalIndices)));
  B.CreateCondBr(IsFinal, Success, LoopStart);

  llvm::verifyModule(*M);
}

void TableCompiler::dump() { M->dump(); }
