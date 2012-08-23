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
    :_src(src), _dest(dest), _srcMsg(srcMsg), _destMsg(destMsg), _subject(subject) {};
    
    int srcID() {return _src;}
    int destId() {return _dest;}
    int srcMsgId() {return _srcMsg;}
    int destMsgId() {return _destMsg;}
    int subjectId() {return _subject;}

    size_t numParams() { return 0 ;}
    int getParam(size_t arg) {return 0;}
    
    string toString() ;
    
    MessageTuple* clone();
private:
    int _src;
    int _dest;
    int _srcMsg;
    int _destMsg;
    int _subject;

};

class FsmSnapshot : public StateSnapshot
{
public:
    FsmSnapshot(int stateId):_stateId(stateId) {}
    ~FsmSnapshot() {}
    
    int curStateId() const { return _stateId ;}
    string toString() { stringstream ss ; ss << _stateId ; return ss.str(); }
    int toInt() { return _stateId; }
    StateSnapshot* clone() ;
private:
    int _stateId;
    
};

 
#endif