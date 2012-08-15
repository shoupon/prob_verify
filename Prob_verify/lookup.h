#ifndef LOOKUP_H
#define LOOKUP_H

#include <vector>
#include <map>
#include <cassert>
#include <stdexcept>
#include <sstream>
#include <string>
using namespace std;

class Lookup
{
private:
    vector<string> _list;    
    map<string,int> _tree;

public:
    Lookup() {}   

    int insert(string name) 
    {
        assert( _tree.size() == _list.size() );
        
        int idx = (int)_tree.size();
        map<string,int>::iterator it = _tree.find(name);
        
        if( it == _tree.end() ) {
            _tree.insert( make_pair(name, idx) );
            _list.push_back(name);
            return idx;
        }
        else
            return it->second;
    }

    int toInt(string key)
    {
        map<string,int>::iterator it = _tree.find(key);
        if( it == _tree.end() )
            return -1;
        else
            return it->second;
    }
    
    string toString(int key)
    {
        if( key < 0 ) {
            throw runtime_error("There is no message associated with id < 0, error must present");
            //return string();
        }
        else if( key >= (int)_list.size() ) {
            stringstream ss ;
            ss << "Unable to find message corresponding to the id = " ;
            ss << key ;
            throw runtime_error(ss.str());
            //return string();
        }
        else
            return _list[key];
    }
};

#endif