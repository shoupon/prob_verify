#ifndef PROB_VERIFY_H
#define PROB_VERIFY_H

#include <vector>
#include <map>
#include <set>

#include "define.h"
#include "fsm.h"
#include "state.h"
#include "statemachine.h"
#include "checker.h"
#include "globalstate.h"
#include "errorstate.h"
#include "stoppingstate.h"

typedef map<GlobalState*, int>                   GSMap;
typedef pair<GlobalState*, int>                  GSMapPair;
typedef map<GlobalState*, int>::iterator         GSMapIter;
typedef map<GlobalState*, int>::const_iterator   GSMapConstIter;
typedef map<string, GlobalState*>                UniqueMap;
typedef pair<string, GlobalState*>               UniqueMapPair;


typedef map<string, int>          GSVecMap;
typedef pair<string, int>         GSVecMapPair;
typedef set<string>               GSVecSet;

class ProbVerifier
{
    vector<StateMachine*> _macPtrs;
    vector<GSMap> _arrClass;
    
    vector<StoppingState*> _errors;
    vector<StoppingState*> _stops;
    vector<StoppingState*> _ends;
    
    GSVecMap _arrRS;
    GSVecMap _arrFinRS;
    GSVecMap _arrFinStart;
    
    GSVecSet _reachable;
    
    GlobalState* _root ;
    int _maxClass;
    int _curClass;
    int _max ; // Used to check livelock
    Checker* _checker;
    
public:
    ProbVerifier():_curClass(0), _max(0) {}
    
    void addMachine(StateMachine* machine) { _macPtrs.push_back(machine); }
    //void setRS(vector<GlobalState*> rs);
    void addSTOP(StoppingState* rs);
    void addEND(StoppingState* end);
    // Add errorState into the list of errorStates
    void addError(StoppingState* es);
    // Add the pointer to the user-specified checker machine
    void addChecker(Checker* chkPtr) { _checker = chkPtr; }
    // Provide the criterion on which the program determines to print out the stopping
    // state trace
    void addPrintStop(bool (*printStop)(GlobalState*, GlobalState*) = 0);
    // The basic procedure, start when all machines are in its initial state
    void start(int maxClass);
    // void start(vector<GlobalState*> initStates);
    void clear();
    
    size_t getNumMachines() { return _macPtrs.size(); }
    vector<StateMachine*> getMachinePtrs() const ;
    
private:
    bool (*_printStop)(GlobalState*, GlobalState*);
    
    bool addToClass(GlobalState* childNode, int prob);
    bool addToReachable(GlobalState* gs);
    GSVecMap::iterator find(GSVecMap& collection, GlobalState* gs);
    GSMap::iterator find(GSMap& collection, GlobalState* gs);
    void insert(GSVecMap& collection, GlobalState* gs)
    { collection.insert( GSVecMapPair(gs->toString(), gs->getProb()) ); }
    void printSeq(const vector<GlobalState*>& seq) ;
    
    bool isError(GlobalState* obj);
    bool isStopping(const GlobalState* obj);
    bool isEnding(const GlobalState* obj);
    bool findMatch(const GlobalState* obj, const vector<StoppingState*>& container) ;
    
    void printStopping(const GlobalState* obj) ;
    void printStep(GlobalState* obj) ;
    void printStat();
    
    bool checkDeadlock( GlobalState* gs );
    bool checkLivelock( GlobalState* gs ) ;
    bool checkError( GlobalState* gs ) ;
};


#endif