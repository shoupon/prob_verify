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

private:
    vector<State*> _states;
    
    string _name;
    Table _stateNames;
};

#endif