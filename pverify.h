#ifndef PROB_VERIFY_H
#define PROB_VERIFY_H

#include <cassert>

#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include "define.h"
#include "fsm.h"
#include "state.h"
#include "statemachine.h"
#include "checker.h"
#include "globalstate.h"
#include "errorstate.h"
#include "stoppingstate.h"

typedef map<GlobalState*, int>                   GSMap;
typedef pair<GlobalState*, int>                  GSMapPair;
typedef map<GlobalState*, int>::iterator         GSMapIter;
typedef map<GlobalState*, int>::const_iterator   GSMapConstIter;
typedef map<string, GlobalState*>                UniqueMap;
typedef pair<string, GlobalState*>               UniqueMapPair;


typedef map<string, int>          GSVecMap;
typedef pair<string, int>         GSVecMapPair;
typedef set<string>               GSVecSet;

class ProbVerifier {
  typedef unordered_map<string, GlobalState*>      GSClass;

  vector<StateMachine*> _macPtrs;
  vector<GSMap> _arrClass;
  vector<GSMap> _all;
  
  vector<StoppingState*> _errors;
  vector<StoppingState*> _stops;
  vector<StoppingState*> _ends;
  
  GSVecMap _arrRS;
  GSVecMap _arrFinRS;
  GSVecMap _arrFinStart;
  
  GSVecSet _reachable;
  
  GlobalState* _root ;
  int _maxClass;
  int _curClass;
  int _max ; // Used to check livelock
  Checker* _checker;
    
public:
  ProbVerifier():_curClass(0), _max(0) {}
  
  void addMachine(StateMachine* machine) { _macPtrs.push_back(machine); }
  //void setRS(vector<GlobalState*> rs);
  void addSTOP(StoppingState* rs);
  void addEND(StoppingState* end);
  // Add errorState into the list of errorStates
  void addError(StoppingState* es);
  // Add the pointer to the user-specified checker machine
  void addChecker(Checker* chkPtr) { _checker = chkPtr; }
  // Provide the criterion on which the program determines to print out the stopping
  // state trace
  void addPrintStop(bool (*printStop)(GlobalState*, GlobalState*) = 0);
  // The basic procedure, start when all machines are in its initial state
  void start(int maxClass);
  // void start(vector<GlobalState*> initStates);
  void clear();
  
  size_t getNumMachines() { return _macPtrs.size(); }
  vector<StateMachine*> getMachinePtrs() const ;
    
private:
  bool (*_printStop)(GlobalState*, GlobalState*);
  
  void printSeq(const vector<GlobalState*>& seq) ;
  
  bool isError(GlobalState* obj);
  bool isStopping(const GlobalState* obj);
  bool isEnding(const GlobalState* obj);
  bool findMatch(const GlobalState* obj, const vector<StoppingState*>& container) ;
  
  void printStopping(const GlobalState* obj) ;
  void printStep(GlobalState* obj) ;
  void printStat();
  void printFinRS();
  
  bool checkDeadlock( GlobalState* gs );
  bool checkLivelock( GlobalState* gs ) ;
  bool checkError( GlobalState* gs ) ;
  void reportDeadlock(GlobalState* gs);
  void reportLivelock(GlobalState* gs);
  void reportError(GlobalState* gs);

  bool isMemberOf(const GlobalState* gs, const vector<string>& container);
  bool isMemberOf(const GlobalState* gs, const GSClass& container);
  bool isMemberOf(const GlobalState* gs, const vector<GSClass>& containers);
  bool isMemberOfClasses(const GlobalState* gs);
  bool isMemberOfEntries(const GlobalState* gs);

  void copyToClass(const GlobalState* gs, int k);
  void copyToEntry(const GlobalState* gs, int k);

  void DFSVisit(GlobalState* gs, int k);
  void stackPush(GlobalState* gs);
  void stackPop();
  void stackPrint();
  bool isMemberOfStack(const GlobalState* gs);

  vector<GSClass> classes_;
  vector<GSClass> entries_;
  vector<string> dfs_stack_string_;
  vector<GlobalState*> dfs_stack_state_;
};


#endif
