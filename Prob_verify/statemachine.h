#ifndef STATEMACHINE_H
#define STATEMACHINE_H

#include <vector>
#include <string>
#include <exception>
#include <stdexcept>
#include <memory>
using namespace std;

#include "define.h"
#include "lookup.h"

class MessageTuple
{
public:
    virtual ~MessageTuple() {}
    virtual int srcID() =0 ;
    virtual int destId() = 0 ;
    virtual int srcMsgId() = 0 ;
    virtual int destMsgId() = 0;
    virtual int subjectId() = 0;

    virtual size_t numParams() = 0;
    virtual int getParam(size_t arg) = 0 ;
    
    virtual string toString() = 0;
    
    virtual MessageTuple* clone() = 0 ;
private:
};



class StateSnapshot;

class StateMachine
{
public:
    // When the class is constructed, a lookup function should be provided to the
    // StateMachine, so the StateMachine can convert a string of an input or an output
    // label into id (integer). A pointer to Parser should be provided to the StateMachine.
    // The Parser class contains the necessary look up function that StateMachine needs
    StateMachine( Lookup* msg, Lookup* mac ): _msgLookup(msg), _macLookup(mac) { }

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
                           bool& high_prob, int startIdx = 0) = 0;
    // Returns the identifier of current state
    virtual int nullInputTrans(vector<MessageTuple*>& outMsgs,
                                  bool& high_prob, int startIdx = 0) = 0;
    // Restore the state of a StateMachine back to a previous point which can be completely specified
    // by s StateSnapshot
    virtual void restore(const StateSnapshot* snapshot) = 0;
    // Store current snapshot. This function should allocate a new StateSnapshot* object. The object would be
    // deallocate after the process of probabilistic verification is complete, when all the StateSnapshot* in the
    // _uniqueTable is released.
    virtual StateSnapshot* curState() = 0 ;
    // Reset the machine to initial state
    virtual void reset() = 0;

protected:
    Lookup* _msgLookup;
    Lookup* _macLookup;
    
    int messageToInt(string msg) { return _msgLookup->toInt(msg); }
    int machineToInt(string macName) { return _macLookup->toInt(macName);}
    string IntToMessage(int id) { return _msgLookup->toString(id); }
    string IntToMachine(int id) { return _macLookup->toString(id); }
};

// Used for restore the state of a state machine back to a certain point. Should contain
// the state in which the machine was, the internal variables at that point
class StateSnapshot
{
    friend class StateMachine;
public:
    virtual ~StateSnapshot() {} ;
    virtual int curStateId() const = 0 ;
    // Returns the name of current state as specified in the input file
    virtual string toString() = 0 ;
    // Cast the Snapshot into a integer. Used in HashTable
    virtual int toInt() = 0;
    virtual StateSnapshot* clone() = 0 ;
};

#endif
