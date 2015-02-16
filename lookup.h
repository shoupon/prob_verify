#ifndef LOOKUP_H
#define LOOKUP_H

#include <cassert>

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <unordered_map>
using namespace std;

class Lookup {
public:
  Lookup() {
    _tree.clear();
    _list.clear();
    insert("null");
  }   

  int insert(string name) {
    assert(_tree.size() == _list.size());
    int idx = (int)_tree.size();
    auto it = _tree.find(name);
    
    if (it == _tree.end()) {
      _tree[name] = idx;
      _list.push_back(name);
      return idx;
    }
    else
      return it->second;
  }

  int toInt(string key) {
    if (_tree.begin() == _tree.end())
      return -1;
    auto it = _tree.find(key);
    if (it == _tree.end())
      return -1;
    else
      return it->second;
  }
  
  string toString(int key) {
    if (key < 0) {
      throw runtime_error(
          "There is no message associated with id < 0, error must present");
    }
    /*
    else if( key >= _list.size() ) {
      stringstream ss;
      ss << "Unable to find message corresponding to the id = ";
      ss << key;
      throw runtime_error(ss.str());
    }*/
    else
      return _list[key];
  }

  void dump() const {
    int k = 0;
    for (auto& s : _list)
      cout << s << " at index " << k++ << endl;
  }

private:
  vector<string> _list;    
  unordered_map<string,int> _tree;
};

#endif
