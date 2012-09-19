#ifndef PROB_VERIFY_H
#define PROB_VERIFY_H

#include <vector>
#include <map>

#include "define.h"
#include "fsm.h"
#include "state.h"
#include "statemachine.h"
#include "globalstate.h"
#include "errorstate.h"

typedef map<GlobalState*, int>                   GSMap;
typedef pair<GlobalState*, int>                  GSMapPair;
typedef map<GlobalState*, int>::iterator         GSMapIter;
typedef map<GlobalState*, int>::const_iterator   GSMapConstIter;

typedef map<vector<string>, int>          GSVecMap;
typedef pair<vector<string>, int>         GSVecMapPair;

class ProbVerifier
{
private:
    vector<StateMachine*> _macPtrs;
    vector<GSMap> _arrClass;
    vector<ErrorState*> _errors;
    // TODO
    GSVecMap _arrFinRS;
    GSVecMap _arrFinStart;
    GSVecMap _RS;
    GlobalState* _root ;
    int _maxClass;
    int _curClass;   
    int _max ; // Used to check livelock

    bool addToClass(GlobalState* childNode, int toClass);
    GSVecMap::iterator find(GSVecMap& collection, GlobalState* gs);
    GSMap::iterator find(GSMap& collection, GlobalState* gs);
    void insert(GSVecMap& collection, GlobalState* gs) 
    { collection.insert( GSVecMapPair(gs->getStringVec(), gs->getProb()) ); }
    void printSeq(const vector<GlobalState*>& seq) ;
    
    ErrorState* isError(const GlobalState* obj);
    
    void printRS() ;

public:
    ProbVerifier():_curClass(0), _max(0) {}

    void addMachine(StateMachine* machine) { _macPtrs.push_back(machine); }
    //void setRS(vector<GlobalState*> rs); 
    void addRS(vector<StateSnapshot*> rs);
    // Add errorState into the list of errorStates
    void addError(ErrorState* es);
    // The basic procedure, start when all machines are in its initial state
    void start(int maxClass);
    // void start(vector<GlobalState*> initStates);
    void clear();
    
    size_t getNumMachines() { return _macPtrs.size(); }
    vector<StateMachine*> getMachinePtrs() const ;
    
};


#endif