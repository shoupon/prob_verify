#include <iostream>
#include <vector>
#include <string>
#include <sstream>
using namespace std;

#ifndef FSM
#define FSM

#include "define.h"
#include "transition.h"
#include "state.h"
#include "statemachine.h"
#include "lookup.h"

class Fsm : public StateMachine
{
public:
    Fsm(string name, Lookup* msg, Lookup* mac);
    
    bool addState(string name);
    bool addTransition(string from, string to, Transition& label);
    bool addTransition(int from, int to, Transition& label);
    State* getState(string name);
    State* getState(int id);
    State* getInitState() { return _states[0]; }        

    // Implement the virtual functions in StateMachine
    int transit(MessageTuple* inMsg, vector<MessageTuple*>& outMsgs,
                   bool& high_prob, int startIdx = 0);
    int nullInputTrans(vector<MessageTuple*>& outMsgs, bool& high_prob, int startIdx = 0);
    void restore(const StateSnapshot* snapshot);
    StateSnapshot* curState();
    void reset() ;    

private:
    vector<State*> _states;
    int _current;
    
    string _name;
    Table _stateNames;
};

class FsmMessage : public MessageTuple
{
public:
    FsmMessage(int src, int dest, int srcMsg, int destMsg, int subject)
    :MessageTuple(src, dest, srcMsg, destMsg, subject) {}
    
    size_t numParams() { return 0 ;}
    int getParam(size_t arg) {return 0;}
    
    string toString() { return MessageTuple::toString(); }
    
    MessageTuple* clone();
private:

};

class FsmSnapshot : public StateSnapshot
{
public:
    FsmSnapshot(int stateId):_stateId(stateId) {}
    ~FsmSnapshot() {}
    
    int curStateId() const { return _stateId ;}
    string toString() { stringstream ss ; ss << _stateId ; return ss.str(); }
    int toInt() { return _stateId; }
    StateSnapshot* clone() const;
private:
    int _stateId;
    
};

 
#endif