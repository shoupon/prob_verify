#include <iostream>
#include <vector>
#include <string>
using namespace std;

#ifndef FSM
#define FSM

#include "define.h"
#include "transition.h"
#include "state.h"

class Fsm : class StateMachine
{
public:
    Fsm(string name);
    
    bool addState(string name);
    bool addTransition(string from, string to, Transition& label);
    bool addTransition(int from, int to, Transition& label);
    State* getState(string name);
    State* getState(int id);
    State* getInitState() { return _states[0]; }    

    void reset() ;

    // Implement the virtual functions in StateMachine
    size_t transit(MessageTuple inMsg, vector<MessageTuple>& outMsgs, bool& high_prob, size_t startIdx = 0);
    size_t nullInputTrans(vector<MessageTuple>& outMsgs, bool& high_prob, size_t startIdx = 0);    
    void restore(StateSnapshot& snapshot);    
    Snapshot curState();  

private:
    vector<State*> _states;
    int _current;
    
    string _name;
    Table _stateNames;
};

class FsmMessage : class MessageTuple
{
public:    
    int srcID() {return _src;}
    int destId() {return _dest;}
    int srcMsgId() {return _srcMsg;}
    int destMsgId() {return _destMsg;}
    int subjectId() {return _subject;}

    size_t numParams() { return 0 ;}
    int getParam(size_t arg) {return 0;}
private:
    int _src;
    int _dest;
    int _srcMsg;
    int _destMsg;
    int _subject;

};

 
#endif