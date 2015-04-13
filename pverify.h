#ifndef PROB_VERIFY_H
#define PROB_VERIFY_H

#include <cassert>
#include <cmath>

#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include "define.h"
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
typedef set<string>                              UniqueSet;
typedef pair<string, GlobalState*>               UniqueMapPair;
typedef unordered_set<int>                       GSClass;
typedef unordered_map<int, GlobalState *>        GSEntry;

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

class ProbVerifier;

#define DFS_BOUND 0
#define TREE_BOUND 1
class ProbVerifierConfig {
public:
  friend ProbVerifier;

  ProbVerifierConfig();
  void setLowProbBound(double p);
  void setBoundMethod(int method);
  void enableTraceback() { trace_back_ = 1; }
  void disableTraceback() { trace_back_ = 0; }

private:
  double low_p_bound_;
  double low_p_bound_inverse_;
  int bound_method_;
  int trace_back_;
};

class ProbVerifier {
  static vector<StateMachine*> machine_ptrs_;
  static Indexer<string> unique_states_;

  vector<GSMap> _arrClass;
  vector<GSMap> _all;
  
  vector<StoppingState*> _errors;
  vector<StoppingState*> _stops;
  vector<StoppingState*> _ends;
  
  GSVecMap _arrRS;
  GSVecMap _arrFinRS;
  GSVecMap _arrFinStart;
  
  GSVecSet _reachable;
  
  int _maxClass;
  int _curClass;
  Checker* _checker;

  struct ProbChoice {
    ProbChoice(int state_idx, int prob)
        : state_idx_(state_idx), probability_(prob) {}
    int state_idx_;
    int probability_;
  };
  struct NonDetChoice {
    vector<ProbChoice> prob_choices;
  };
    
public:
  ProbVerifier():_curClass(0) {}
  ~ProbVerifier();
  
  void addMachine(StateMachine* machine);
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
  static StateMachine* getMachine(const string& machine_name);
  void configure(const ProbVerifierConfig& cfg) { config_ = cfg; }
  // the boilerplate to be setup before starting the whole model checking
  // procedure
  void initialize(const GlobalState* init_state);
  // The basic procedure, start when all machines are in its initial state
  void start(int max_class);
  void start(int max_class, const GlobalState* init_state);
  void start(int max_class, int verbose);
  void start(int max_class, const GlobalState* init_state, int verbose);
  // void start(vector<GlobalState*> initStates);
  double computeBound(int target_class);
  bool findCycle();
  void clear();
  
  static size_t getNumMachines() { return machine_ptrs_.size(); }
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
  void printContent(const vector<GSEntry>& containers);
  void printContent(const GSEntry& container);
  void printStat();
  void printStat(int class_k);
  void printStoppings();
  void printEndings();
  void printFinRS();
  
  bool hasProgress(GlobalState* gs);
  void reportDeadlock(GlobalState* gs);
  void reportLivelock(GlobalState* gs);
  void reportError(GlobalState* gs);

  static int stateToIndex(const string& state_str);
  static int stateToIndex(const GlobalState* gs);
  static string indexToState(int idx);

  bool isMemberOf(const GlobalState* gs, const vector<string>& container);
  bool isMemberOf(const string& s, const vector<string>& container);
  bool isMemberOf(const GlobalState* gs, const vector<int>& container);
  bool isMemberOf(int state_idx, const vector<int>& container);
  bool isMemberOf(const GlobalState* gs, const GSClass& container);
  bool isMemberOf(const string& s, const GSClass& container);
  bool isMemberOf(int state_idx, const GSClass& container);
  bool isMemberOf(const GlobalState* gs, const GSEntry& container);
  bool isMemberOf(const string& s, const GSEntry& container);
  bool isMemberOf(int state_idx, const GSEntry& container);
  int isMemberOf(const GlobalState* gs, const vector<GSClass>& containers);
  int isMemberOf(const string& s, const vector<GSClass>& containers);
  int isMemberOf(int state_idx, const vector<GSClass>& containers);
  int isMemberOf(const GlobalState* gs, const vector<GSEntry>& containers);
  int isMemberOf(const string& s, const vector<GSEntry>& containers);
  int isMemberOf(int state_idx, const vector<GSEntry>& containers);
  int isMemberOfClasses(const GlobalState* gs);
  int isMemberOfClasses(const string& s);
  int isMemberOfClasses(int state_idx);
  int isMemberOfEntries(const GlobalState* gs);
  int isMemberOfEntries(const string& s);
  int isMemberOfEntries(int state_idx);

  void copyToClass(const GlobalState* gs, int k);
  void copyToEntry(const GlobalState* gs, int k);
  void copyToExploredEntry(const GlobalState* gs, int k);

  void DFSVisit(GlobalState* gs, int k);
  double DFSComputeBound(int state_idx, int limit);
  int treeComputeBound(int state_idx, int depth, int limit);
  bool DFSFindCycle(int state_idx);
  void addProbChoice(NonDetChoice &ndc, const GlobalState *child);
  void addProbChoice(NonDetChoice &ndc, const GlobalState *child, int prob);
  void addNonDetChoice(const GlobalState *parent, const NonDetChoice &ndc);
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

  ProbVerifierConfig config_;

  vector<GSClass> classes_;
  vector<GSEntry> entries_;
  vector<GSEntry> explored_entries_;
  vector<int> dfs_stack_indices_;
  vector<GlobalState*> dfs_stack_state_;
  unordered_set<int> reached_stoppings_;
  unordered_set<int> reached_endings_;
  unordered_map<int, vector<NonDetChoice>> nd_choices_;
  unordered_map<int, double> alphas_;
  unordered_set<int> visited_;
  vector<double> inverse_ps_;
  double alpha_diff_;

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
