#ifndef PROBVERIFY_INDEXER_H
#define PROBVERIFY_INDEXER_H

#include <cassert>

#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <unordered_map>
using namespace std;

/* 
 * T should be hashable, i.e. could be the key of unordered_map
 */
template <typename T>
class Indexer {
public:
  Indexer() {
    _tree.clear();
    _list.clear();
  }   

  int insert(const T& item) {
    assert(_tree.size() == _list.size());
    int idx = (int)_tree.size();
    auto it = _tree.find(item);
    
    if (it == _tree.end()) {
      _tree[item] = idx;
      _list.push_back(item);
      return idx;
    }
    else
      return it->second;
  }

  int getIndex(const T& key) {
    if (_tree.begin() == _tree.end())
      return -1;
    auto it = _tree.find(key);
    if (it == _tree.end())
      return -1;
    else
      return it->second;
  }
  
  T getItem(int idx) {
    if (idx < 0) {
      throw runtime_error(
          "There is no item associated with index < 0");
    }
    else
      return _list[idx];
  }

private:
  vector<T> _list;    
  unordered_map<T,int> _tree;
};

#endif // PROBVERIFY_INDEXER_H
