#ifndef PROB_VERIFY_H
#define PROB_VERIFY_H

#include <vector>
#include <map>

#include "define.h"
#include "fsm.h"
#include "state.h"
#include "globalstate.h"

typedef map<GlobalState*, int>                   GSMap;
typedef pair<GlobalState*, int>                  GSMapPair;
typedef map<GlobalState*, int>::iterator         GSMapIter;
typedef map<GlobalState*, int>::const_iterator   GSMapConstIter;

typedef map<vector<int>, int>                    GSVecMap;
typedef pair<vector<int>, int>                   GSVecMapPair;

class ProbVerifier
{
private:
    vector<Fsm*> _macPtrs;
    vector<GSMap> _arrClass;
    vector<GSMap> _computedClass;
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
    { collection.insert( GSVecMapPair(gs->getStateVec(), gs->getProb()) ); }
    


public:
    ProbVerifier():_curClass(0), _max(0) {}

    void addFsm(Fsm* machine) { _macPtrs.push_back(machine); }
    //void setRS(vector<GlobalState*> rs); 
    void addRS(vector<int> rs);
    // The basic procedure, start when all machines are in its initial state
    void start(int maxClass);
    // void start(vector<GlobalState*> initStates);
};



#endif