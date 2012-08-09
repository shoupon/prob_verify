#ifndef STATEMACHINE_H
#define STATEMACHINE_H

#include "define.h"
#include "state.h"
#include "transition.h"
#include "fsm.h"

#include <vector>
#include <string>

class MessageTuple
{
public:    
    int srcID() = 0;
    int destId() = 0 ;
    int srcMsgId() = 0;
    int destMsgId() = 0;

    size_t numParams() = 0;
    int getParam(size_t arg);
private:
};

typedef map<int,MessageTuple>            OutMessageMap ;
typedef map<int,MessageTuple>::iterator  OutMessageMapIter;
typedef pair<int,MessageTuple>           OutMessage;   // ToMachine.id ! message
typedef pair<int,MessageTuple>           InMessage;

class StateMachine
{
public:
    // When the class is constructed, a lookup function should be provided to the StateMachine,
    // so the StateMachine can convert a string of an input or an output label into id (integer)
    virtual StateMachine( int (*lookupMachine)(string), int (*lookupMsg)(string) ) = 0 ;

    virtual size_t transit(MessageTuple inMsg, vector<MessageTuple>& outMsgs, 
                           bool& high_prob, size_t startIdx = 0) = 0 ;
    // Returns the identifier of current state
    virtual size_t nullInputTrans(vector<MessageTuple>& outMsgs, 
                                  bool& high_prob, size_t startIdx = 0) = 0 ;
    
    // Restore the state of a StateMachine back to a previous point which can be completely specified
    // by s StateSnapshot
    virtual void restore(StateSnapshot& snapshot) = 0;
    // Store current snapshot
    virtual Snapshot curState();

private:

    int (*_lookupMachine)(string);
    int (*_lookupMessage)(string);
};

// Used for restore the state of a state machine back to a certain point. Should contain
// the state in which the machine was, the internal variables at that point
abstract class StateSnapshot
{
    friend class StateMachine;
public:
    virtual int curStateId() = 0 ;
    // Returns the name of current state as specified in the input file
    virtual string toString() = 0 ;
    // Return number of null input transitions
};

#endif