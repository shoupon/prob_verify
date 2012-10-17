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
#include "stoppingstate.h"

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
    
    vector<StoppingState*> _errors;
    vector<StoppingState*> _RS;

    GSVecMap _arrRS;
    GSVecMap _arrFinRS;
    GSVecMap _arrFinStart;
    GlobalState* _root ;
    int _maxClass;
    int _curClass;   
    int _max ; // Used to check livelock
    
    bool (*_printStop)(GlobalState*, GlobalState*);

    bool addToClass(GlobalState* childNode, int toClass);
    GSVecMap::iterator find(GSVecMap& collection, GlobalState* gs);
    GSMap::iterator find(GSMap& collection, GlobalState* gs);
    void insert(GSVecMap& collection, GlobalState* gs) 
    { collection.insert( GSVecMapPair(gs->getStringVec(), gs->getProb()) ); }
    void printSeq(const vector<GlobalState*>& seq) ;
    
    bool isError(const GlobalState* obj);
    bool isStopping(const GlobalState* obj);
    bool findMatch(const GlobalState* obj, const vector<StoppingState*>& container) ;


public:
    ProbVerifier():_curClass(0), _max(0) {}

    void addMachine(StateMachine* machine) { _macPtrs.push_back(machine); }
    //void setRS(vector<GlobalState*> rs); 
    void addRS(StoppingState* rs);
    // Add errorState into the list of errorStates
    void addError(StoppingState* es);
    // Provide the criterion on which the program determines to print out the stopping
    // state trace
    void addPrintStop(bool (*printStop)(GlobalState*, GlobalState*) = 0);
    // The basic procedure, start when all machines are in its initial state
    void start(int maxClass);
    // void start(vector<GlobalState*> initStates);
    void clear();
    
    size_t getNumMachines() { return _macPtrs.size(); }
    vector<StateMachine*> getMachinePtrs() const ;
};


#endif