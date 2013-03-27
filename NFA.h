#include <map>
#include <set>
#include <stack>
#include "DFATable.h"

#include <fstream>

class NFA {
public:
  NFA(bool isFinal = false) : isFinal(isFinal) {}
  void addEdge(char C, NFA *N) { Edges.insert(std::pair<char, NFA *>(C, N)); }

  bool hasEdge(char C) { return Edges.find(C) != Edges.end(); }

  NFA *traverseEdge(char C) { return Edges[C]; }

  std::map<char, NFA *> Edges;
  static const char Epsilon = ~0;

  void dump() {
    std::set<NFA *> Visited;
    std::set<NFA *> ToVisit;
    std::ofstream Out("NFA.dot");
    Out << "digraph NFA {";
    ToVisit.insert(this);
    while (ToVisit.size()) {
      NFA *N = *ToVisit.begin();
      ToVisit.erase(ToVisit.begin());
      N->dumpInner(Out, ToVisit, Visited);
    }
    Out << "}";
    Out.close();
  }

  void dumpInner(std::ofstream &Out, std::set<NFA *> &ToVisit,
                 std::set<NFA *> &Visited) {
    Visited.insert(this);

    for (auto &E : Edges) {
      Out << (long)(void *)this << "->" << (long)(void *)E.second
          << "[label=" << E.first << "];";
      if (E.second->isFinal) {
        Out << (long)(void *)E.second << "[shape=doublecircle];";
      }
      if (!Visited.count(E.second))
        ToVisit.insert(E.second);
    }
  }

  bool isFinal = false;
};

class DFAGenerator {
public:
  DFAGenerator(NFA &N) : N(N), D() {}
  void gen();
  DFATable get();
private:
  std::set<NFA *> genEClosure(NFA &N);
  std::set<NFA *> genEClosure(std::set<NFA *> &&N);
  std::set<NFA *> delta(std::set<NFA *> Q, char C);
  bool hasFinal(std::set<NFA *> &N);
  unsigned getID(const std::set<NFA *> &N);
  unsigned NextID = 0;
  NFA &N;
  std::map<std::set<NFA *>, unsigned> IDMap;
  DFATable D;
};

DFATable DFAGenerator::get() { return D; }

std::set<NFA *> DFAGenerator::genEClosure(NFA &N) {
  std::set<NFA *> States;
  States.insert(&N);
  for (auto &E : N.Edges)
    if (E.first == NFA::Epsilon)
      States.insert(E.second);
  return States;
}

std::set<NFA *> DFAGenerator::genEClosure(std::set<NFA *> &&N) {
  std::set<NFA *> States;
  for (auto &NF : N) {
    auto tmp = genEClosure(*NF);
    States.insert(tmp.begin(), tmp.end());
  }
  return States;
}

std::set<NFA *> DFAGenerator::delta(std::set<NFA *> Q, char C) {
  std::set<NFA *> t;
  for (auto &N : Q)
    if (N->hasEdge(C))
      t.insert(N->traverseEdge(C));
  return t;
}

unsigned DFAGenerator::getID(const std::set<NFA *> &N) {
  if (IDMap.find(N) != IDMap.end())
    return IDMap[N];
  IDMap[N] = NextID;
  return NextID++;
}

bool DFAGenerator::hasFinal(std::set<NFA *> &N) {
  for (auto &NF : N) {
    if (NF->isFinal)
      return true;
  }
  return false;
}

static std::vector<char> getAlphabet() {
  std::vector<char> Alphabet;
  Alphabet.push_back('j');
  Alphabet.push_back('o');
  Alphabet.push_back('e');
  Alphabet.push_back('y');
  return Alphabet;
}

void DFAGenerator::gen() {
  std::vector<char> Alphabet = getAlphabet();
  std::stack<std::set<NFA *> > WorkList;
  std::set<unsigned> Q;
  std::set<NFA *> q0 = genEClosure(N);
  Q.insert(getID(q0));
  WorkList.push(q0);
  while (WorkList.size()) {
    std::set<NFA *> q = WorkList.top();
    WorkList.pop();

    for (char C : Alphabet) {
      std::set<NFA *> t = genEClosure(delta(q, C));
      bool hasFinalState = hasFinal(t);

      unsigned tID = getID(t);
      if (hasFinalState)
        tID |= DFATable::Final;

      D.set(getID(q), C, tID);
      if (!Q.count(getID(t))) {
        Q.insert(getID(t));
        WorkList.push(t);
      }
    }
  }
}
