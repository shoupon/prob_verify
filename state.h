#include <vector>
#include <string>
#include <cassert>
using namespace std;

#ifndef STATE_H
#define STATE_H

#include "transition.h"

class State;
typedef pair<State*,bool> EdgeProb;
typedef pair<int, State*> Arrow;

class State 
{
private:
    vector<Transition> _trans;
    vector<State*> _nexts;
    vector<bool> _actives;
    int _stateID;
    string _name;

    void getNextState(int from, int input, vector<int>& nexts, vector<bool>& prob_high, 
                        vector<Transition>& trans) ;

public:
    State(string name, int id): _stateID(id), _name(name) {;}
    bool operator==(const State &other) const;
    
    int getID() { return _stateID ; }
    Transition getTrans(int id) { return _trans[id]; }
    State* getNextState(int id) { return _nexts[id]; }
    size_t getNumTrans() { return _trans.size() ; }
    int isActive() ;

    void deactivate(int id) { assert(id>=0 && id < (int)_actives.size()); _actives[id] = false; }
    void reset() { }

    // Return the pointer of next state of the machine (receiver) which executes the transition that 
    // matches the transition of the this machine (sender). Return null pointer if the machine does not 
    // have matching
    // A transition can possibly match two states, one with high probability transition and another 
    // with lower one. Therefore, references of vector should be passed into the function
    // All the possible next states and their corresponding probability will be returned with the 
    // vector references
    void match(int sndrID, int msgID, State* rcvrStatePtr, vector<EdgeProb>& result) ;
    //void match(int id, State* rcvrStatePtr, vector<EdgeProb>& result) 
    //    { match(_trans[id], rcvrStatePtr, result); }
    void receive(int from, int message, vector<Arrow>& matched );

    void addOutEdge(Transition edge, State* nextState );

    bool findTransition (const Transition& label) const;
};


#endif