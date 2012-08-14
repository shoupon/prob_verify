#ifndef STATEMACHINE_H
#define STATEMACHINE_H

#include "define.h"
#include "state.h"
#include "transition.h"
#include "fsm.h"
#include "parser.h"

#include <vector>
#include <string>

class MessageTuple
{
public:    
    virtual int srcID() = 0;
    virtual int destId() = 0 ;   
    virtual int srcMsgId() = 0;
    virtual int destMsgId() = 0;
    virtual int subjectId() = 0 ;

    virtual size_t numParams() = 0;
    virtual int getParam(size_t arg) = 0;
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
    // A pointer to Parser should be provided to the StateMachine. The Parser class contains the
    // necessary look up function that StateMachine needs
    StateMachine( Parser* ptr ): _psrPtr(ptr) { }

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

protected:
    Parser* _psrPtr;
    int messageToInt(string msg) { checkPsrPtr(); _psrPtr->messageToInt(msg); }
    int machineToInt(string macName) { checkPsrPtr(); _psrPtr->machineToInt(macName);]
    string IntToMessage(int id) { checkPsrPtr(); _psrPtr->IntToMessage(id); }
    string IntToMachine(int id) { checkPsrPtr(); _psrPtr->IntToMachine(id); }

    bool checkPsrPtr() { if( _psrPtr == 0 ) throw runtime_error("_psrPtr is not initialized."); }
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
    // Cast the Snapshot into a integer. Used in HashTable
    virtual int toInt() = 0;
};

#endif