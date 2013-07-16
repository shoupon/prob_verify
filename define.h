#include <map>
using namespace std;

#ifndef DEFINE_H
#define DEFINE_H

#define ALLOW_UNMATCHED

#define TRACE
#ifdef TRACE
#define TRACE_UNMATCHED
//#define TRACE_STOPPING
#endif

//#define CONTROLLER_TIME

// Debugging flags
//#define VERBOSE
//#define VERBOSE_EVAL
//#define VERBOSE_LIVELOCK
//#define VERBOSE_ACTIONS
#ifdef VERBOSE
#define LOG
#endif

#define LOG

typedef map<string,int> Table;
typedef Table::value_type Entry;
typedef pair<Table::iterator, bool> InsertResult;

#endif