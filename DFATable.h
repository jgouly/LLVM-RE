#ifndef DFATABLE_H
#define DFATABLE_H
#include <map>
#include <set>
#include <vector>

class DFATable {
  typedef std::vector<std::map<char, unsigned> > StatesMapTy;
public:
  DFATable(StatesMapTy && States) : States(States) {}
  DFATable() : States() {}

  void set(unsigned From, char At, unsigned To) {
    if (States.size() <= From)
      States.resize(From + 1);
    States[From][At] = To;
  }

  const std::set<char> getUniqueChars() {
    std::set<char> Uniq;
    for (auto &Row : States) {
      for (auto &Cell : Row) {
        Uniq.insert(Cell.first);
      }
    }
    return Uniq;
  }

  StatesMapTy States;
  static const unsigned Final = 1 << ((sizeof(unsigned) * 8) - 1);
};
#endif
