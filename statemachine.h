#ifndef STATEMACHINE_H
#define STATEMACHINE_H

#include <vector>
#include <string>
#include <exception>
#include <stdexcept>
#include <memory>
#include <sstream>
using namespace std;

#include "define.h"
#include "lookup.h"

class MessageTuple;

// Used for restore the state of a state machine back to a certain point.
// Should contain the information that is necessary to restore a machine back to
// a certain state
class StateSnapshot {
  friend class StateMachine;
public:
  StateSnapshot(): _ss_state(0) {}
  StateSnapshot(int s):_ss_state(s) {}
  virtual ~StateSnapshot() {}
  virtual int curStateId() const { return _ss_state; }
  // Returns the name of current state as specified in the input file. Used to identify
  // states in the set STATET, STATETABLE, RS
  virtual string toString() const;
  virtual string toReadable() const { return this->toString(); }
  // Cast the Snapshot into a integer. Used in HashTable
  virtual int toInt() {return _ss_state;}
  virtual StateSnapshot* clone() const { return new StateSnapshot(*this); }
  virtual bool match(StateSnapshot* other) { return toString() == other->toString(); }
  
protected:
  int _ss_state;
};

class StateMachine
{
  static Lookup machine_lookup_;
  static Lookup message_lookup_;
public:
  StateMachine() {}

  // Legacy contructor, takes two Lookup tables and does nothing; obsolete
  StateMachine(Lookup* msg, Lookup* mac) {} 
  virtual ~StateMachine() {}

  // Simulate message reception
  // inMsg: the message transmitted to this StateMachine
  // outMsgs: the output messages generated by the matching transition
  // startIdx: the starting idx from which the search for matching transition starts
  // This function should return the starting index for the next search. A negative
  // return value is used to indicate failure to find a matching transition
  
  // Note: when creating tasks, the tasks should be cloned instead of copy the pointer.
  // There may be same tasks in different childs created by multiple low probability
  // transition. The task (tuple) will be deleted once it is evaluated in evaluate().
  // Cloning tasks avoids dereference of deallocated objects when the same tasks are
  // being evaluated in multiple childs
  virtual int transit(MessageTuple* inMsg, vector<MessageTuple*>& outMsgs,
                      bool& high_prob, int startIdx = 0) { return -1; }
  virtual int transit(MessageTuple* in_msg, vector<MessageTuple*>& out_msgs,
                      int& prob_level, int start_idx);
  int transit(MessageTuple* in_msg, vector<MessageTuple*>& out_msgs,
              int& prob_level);
  // Returns the identifier of current state
  virtual int nullInputTrans(vector<MessageTuple*>& outMsgs,
                             bool& high_prob, int startIdx = 0) { return -1; }
  virtual int nullInputTrans(vector<MessageTuple*>& out_msgs,
                             int& prob_level, int start_idx);
  int nullInputTrans(vector<MessageTuple*>& out_msgs, int& prob_level);
  // Restore the state of a StateMachine back to a previous point which can be
  // completely specified by a StateSnapshot
  virtual void restore(const StateSnapshot* snapshot) { _state = snapshot->curStateId(); }
  // Store current snapshot. This function should allocate a new StateSnapshot* object.
  // The object would be deallocate after the process of probabilistic verification is
  // complete, when all the StateSnapshot* in the _uniqueTable is released.
  virtual StateSnapshot* curState() { return new StateSnapshot(_state); }
  // Reset the machine to initial state
  virtual void reset() { _state = 0 ; }
  virtual string getName() const { return machine_name_; }
  
  int macId() const { return _machineId; }
  void setId(int num) { _machineId = num; }
  int getState() const { return _state; }
  
  //static void setLookup(Lookup* msg, Lookup* mac) { _msgLookup = msg, _macLookup = mac;}
  // Legacy function does nothing
  static void setLookup(Lookup* msg, Lookup* mac) {}
  
  static int messageToInt(const string& msg);
  static int machineToInt(const string& machine_name);
  static string IntToMessage(int num);
  static string IntToMachine(int num);
  static void dumpMachineTable();
  /*
  static string IntToMessage(int num) { return _msgLookup->toString(num); }
  static string IntToMachine(int num) { return _macLookup->toString(num); }
  */

protected:
  MessageTuple* createOutput(MessageTuple* in_msg, int dest, int dest_msg);
  MessageTuple* createOutput(MessageTuple* in_msg, const string& dest_name,
                             const string& dest_msg_name);
  //static Lookup* _msgLookup;
  //static Lookup* _macLookup;
  int _state;
  string machine_name_;
  
private:
  int _machineId;
};

class MessageTuple {
public:
  MessageTuple(int src, int dest, int srcMsg, int destMsg, int subject)
  :_src(src), _dest(dest), _srcMsg(srcMsg), _destMsg(destMsg), _subject(subject) {}
  
  virtual ~MessageTuple() {}
  
  int srcID() {return _src;}
  int destId() {return _dest;}
  int srcMsgId() {return _srcMsg;}
  int destMsgId() {return _destMsg;}
  int subjectId() {return _subject;}

  virtual size_t numParams() { return 0 ; }
  virtual int getParam(size_t arg) { return 0 ; }
  
  virtual string toString() const {
    stringstream ss;
    ss << "subject=" << _subject << ":" ;
    ss << "(" << _src << "?" << _srcMsg << "," << _dest << "!" << _destMsg << ")" ;
    return ss.str() ;
  }

  virtual string toReadable() const {
    stringstream ss;
    ss << "message " << StateMachine::IntToMessage(_destMsg)
       << " (" << this->readableParams() << ") "
       << "from " << StateMachine::IntToMachine(_subject)
       << " to " << StateMachine::IntToMachine(_dest);
    return ss.str();
  }

  virtual string readableParams() const {
    return "";
  }
  
  virtual MessageTuple* clone() const {return new MessageTuple(*this); }
  
  virtual bool operator==(const MessageTuple& rhs) const;
  virtual bool operator<(const MessageTuple &rhs) const;
protected:
  int _src;
  int _dest;
  int _srcMsg;
  int _destMsg;
  int _subject;
};




#endif
