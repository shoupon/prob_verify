#include <iostream>
#include <vector>
#include <string>
using namespace std;

#ifndef FSM
#define FSM

#include "define.h"
#include "transition.h"
#include "state.h"
#include "statemachine.h"
#include "parser.h"

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

    void reset() ;

    // Implement the virtual functions in StateMachine
    size_t transit(MessageTuple inMsg, vector<MessageTuple>& outMsgs, bool& high_prob, size_t startIdx = 0);
    size_t nullInputTrans(vector<MessageTuple>& outMsgs, bool& high_prob, size_t startIdx = 0);    
    void restore(StateSnapshot& snapshot);
    StateSnapshot curState();

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
private:
    int _src;
    int _dest;
    int _srcMsg;
    int _destMsg;
    int _subject;

};

 
#endif