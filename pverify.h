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
#include "indexer.h"
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

class ProtocolError {
public:
  ProtocolError(const string& s): error_msg_(s) {}
  string toString() { return string("ProtocolError: ") + error_msg_; }

  const static ProtocolError kDeadLock;
  const static ProtocolError kLivelock;
  const static ProtocolError kErrorState;
  const static ProtocolError kCheckerError;
private:
  string error_msg_;
};

class UnspecifiedReception : public ProtocolError {
public:
  UnspecifiedReception(): ProtocolError("unspecified reception") {}
  UnspecifiedReception(const MessageTuple* msg)
      : ProtocolError("unspecified reception of " + msg->toReadable()) {
    ;
  }
  ~UnspecifiedReception() {}
};

class ProbVerifier {
  typedef unordered_map<int, GlobalState*>      GSClass;

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
  // the boilerplate to be setup before starting the whole model checking
  // procedure
  void initialize(const GlobalState* init_state);
  // The basic procedure, start when all machines are in its initial state
  void start(int max_class);
  void start(int max_class, const GlobalState* init_state);
  void start(int max_class, int verbose);
  void start(int max_class, const GlobalState* init_state, int verbose);
  // void start(vector<GlobalState*> initStates);
  int computeBound(int target_class, double inverse_p);
  int computeBound(int target_class, double inverse_p, bool dfs);
  bool findCycle();
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
  
  void printStep(GlobalState* obj) ;

  void resetStat();
  void printContent(const vector<GSClass>& containers);
  void printContent(const GSClass& container);
  void printStat();
  void printStat(int class_k);
  void printStoppings();
  void printEndings();
  void printFinRS();
  
  bool hasProgress(GlobalState* gs);
  void reportDeadlock(GlobalState* gs);
  void reportLivelock(GlobalState* gs);
  void reportError(GlobalState* gs);

  int stateToIndex(const string& state_str);
  int stateToIndex(const GlobalState* gs);
  string indexToState(int idx);

  bool isMemberOf(const GlobalState* gs, const vector<string>& container);
  bool isMemberOf(const string& s, const vector<string>& container);
  bool isMemberOf(const GlobalState* gs, const vector<int>& container);
  bool isMemberOf(int state_idx, const vector<int>& container);
  GlobalState* isMemberOf(const GlobalState* gs, const GSClass& container);
  GlobalState* isMemberOf(const string& s, const GSClass& container);
  GlobalState* isMemberOf(int state_idx, const GSClass& container);
  GlobalState* isMemberOf(const GlobalState* gs, const vector<GSClass>& containers);
  GlobalState* isMemberOf(const string& s, const vector<GSClass>& containers);
  GlobalState* isMemberOf(int state_idx, const vector<GSClass>& containers);
  GlobalState* isMemberOfClasses(const GlobalState* gs);
  GlobalState* isMemberOfClasses(const string& s);
  GlobalState* isMemberOfClasses(int state_idx);
  GlobalState* isMemberOfEntries(const GlobalState* gs);
  GlobalState* isMemberOfEntries(const string& s);
  GlobalState* isMemberOfEntries(int state_idx);

  GlobalState* copyToClass(const GlobalState* gs, int k);
  GlobalState* copyToEntry(const GlobalState* gs, int k);
  GlobalState* copyToExploredEntry(const GlobalState* gs, int k);

  void DFSVisit(GlobalState* gs, int k);
  int DFSComputeBound(int state_idx, int limit);
  int treeComputeBound(int state_idx, int depth, int limit);
  bool DFSFindCycle(int state_idx);
  void addChild(const GlobalState* par, const GlobalState* child);
  void addChild(const GlobalState* par, const GlobalState* child, int prob);
  void stackPush(GlobalState* gs);
  void stackPop();
  void stackPrint();
  // print the content of dfs_stack_string_ from the state string that reads
  // "from" until the end of the stack. [from, end)
  void stackPrintFrom(const string& from);
  // print the content of dfs_stack_string_ from the start of the stack until
  // the state string reads "until". does not print until [begin, until)
  void stackPrintUntil(const string& until);
  void printArrowStateNewline(const GlobalState* gs);
  bool isMemberOfStack(const GlobalState* gs);

  vector<GSClass> classes_;
  vector<GSClass> entries_;
  vector<GSClass> explored_entries_;
  vector<int> dfs_stack_indices_;
  vector<GlobalState*> dfs_stack_state_;
  unordered_set<int> reached_stoppings_;
  unordered_set<int> reached_endings_;
  Indexer<string> unique_states_;
  struct Transition {
    Transition(int state_idx, int prob)
        : state_idx_(state_idx), probability_(prob) {}
    int state_idx_;
    int probability_;
  };
  unordered_map<int, vector<Transition>> transitions_;
  unordered_map<int, int> alphas_;
  unordered_set<int> visited_;
  vector<double> inverse_ps_;
  bool alpha_modified_;

  unique_ptr<GlobalState> start_point_;
  unique_ptr<StoppingState> default_stopping_;
  // TODO(shoupon): use preprocessor to create if expression that prints
  // depending on verbosity
  int verbosity_;

  int num_transitions_;
  int stack_depth_;

  const bool log_alpha_evaluation_ = false;
};


#endif
