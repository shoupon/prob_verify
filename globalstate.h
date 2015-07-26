#ifndef GLOBALSTATE_H
#define GLOBALSTATE_H

#include <vector>
#include <cassert>
#include <string>
#include <queue>
#include <set>
#include <list>
#include <unordered_set>

#include "statemachine.h"
#include "checker.h"
#include "service.h"

class GlobalStateHashKey;
class GSVecHashKey;
class GlobalState;

// A vector of int defines the states of machines in this global state using state number
// An integer represents the probability class of global state transition
typedef pair<vector<int>,int> GSProb;

class GlobalState {
  // The ID's of states in a global state
  // A table stores all of the reachable globalState. Each reachable globalState is
  // unique. The _uniqueTable stores the pointers to each unique GlobalState
  static GlobalState* _root;
  
  vector<GlobalState*> _childs;
  queue<MessageTuple*> _fifo;

  int _depth;
public:
  // Default constructor creates a global state with all its machine set to initial
  // state 0.
  GlobalState() { init(); }
  // This copy constructor is used to create childs, 
  // it will automatically increase the distance of childs
  GlobalState(GlobalState* gs);
  GlobalState(const GlobalState* gs);
  // This constructor is used to create a new GlobalState by specifying 
  // the state of its individual machines
  GlobalState(vector<StateSnapshot*>& stateVec) ;
  // *** This constructor should be called first. It will set the static member of
  // GlobalState, such as number of state machines, the pointers to machines, etc.
  GlobalState(vector<StateMachine*> macs, CheckerState* chkState = 0);
  ~GlobalState();

  GlobalState* getChild (size_t i);
  vector<GlobalState *> getChildren() const { return _childs; }
  void removeChildren(GlobalState* child);
  int getProb() const { return _depth; }
  void setProb(int p) { _depth = p; }
  const vector<StateSnapshot*> getStateVec() const { return _gStates ;}

  CheckerState* chkState() { return _checker; }
  const vector<string> getStringVec() const;
  const size_t size() { return _childs.size(); }
  size_t snapshotSize() const;
  size_t getBytes() const;
  bool hasChild() { return size()!=0; }
  bool isBusy() { return !_fifo.empty();}

  void findSucc(vector<GlobalState*>& successors);
  void clearSucc();

  // Take the states of machines as stored in _gStates
  void restore();
  // Save the current state of machines to _gStates
  void store();
  void mutateState(const StateSnapshot* snapshot, int mac_id);

  string toString() const;
  string toReadable() const;
  string toReadableMachineName() const;

  void setRoot() { _root = this; }
  static bool init(GlobalState*);
  static void clearAll() ;

  static void setService(Service* srvc);

  void setTrail(const vector<int>& t) { trail_ = t; }
  int getTrailSize() const { return trail_.size(); }
  int printTrail(string (*translation)(int)) const;
  void setPathCount(int c) { path_count_ = c; }
  void increasePathCount(int increment) { path_count_ += increment; }
  int getPathCount() { return path_count_; }

protected:
  // Number of machines
  static int _nMacs;
  // array of ptr to machines
  static vector<StateMachine*> _machines;
  static Service* _service;
  
  vector<StateSnapshot*> _gStates;
  CheckerState* _checker ;
  ServiceSnapshot* _srvcState;
private:
  //void trim();
  void addParents(const vector<GlobalState*>& arr);
  void eraseChild(size_t idx);
  //void execute(int macId, int transId, Transition* transPtr);
  void evaluate();
  void collapse(GlobalState *nd_root);
  // Invoke explore when an unexplored transition is push onto the queue
  void explore(int subject);
  void addTask(vector<MessageTuple*> msgs);
  bool hasTasks() { return !_fifo.empty(); }
  
  void init() ;
  bool active();
  void recordProb();

  static void printSeq(const vector<GlobalState*>& seq);
  
  string msg2str(MessageTuple* msg);
  vector<int> trail_;
  int path_count_;

  bool blocked_;
};
#endif
