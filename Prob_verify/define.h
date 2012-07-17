#include <map>
using namespace std;

#ifndef DEFINE_H
#define DEFINE_H

//#define HALF_DUPLEX
#define FULL_DUPLEX

typedef map<string,int> Table;
typedef Table::value_type Entry;
typedef pair<Table::iterator, bool> InsertResult;

#endif