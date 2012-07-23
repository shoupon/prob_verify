#ifndef GLOBAL_H
#define GLOBAL_H

#include <vector>
#include <cassert>
#include <string>
#include <queue>

#include "fsm.h"
#include "state.h"
#include "myHash.h"

class GlobalStateHashKey;
class GlobalState;

typedef Hash<GlobalStateHashKey, GlobalState*> GSHash;
typedef Hash<GlobalStateHashKey, vector<int>> GSVecHash;

// A vector of int defines the states of machines in this global state using state number
// An integer represents the probability class of global state transition
typedef pair<vector<int>,int> GSProb;

class Matching
{
public:
    OutLabel _outLabel ;
    int _source;
    int _transId;

    Matching(OutLabel lbl, int s, int transId):_outLabel(lbl), _source(s),_transId(transId) {}
};

class GlobalState
{
private:
    // Number of machines
    static int _nMacs ;
    // array of ptr to machines
    static vector<Fsm*> _machines;    
    // The ID's of states in a global state
    // A table stores all of the reachable global states
    static GSHash _uniqueTable ;
    vector<int> _gStates;     
    
    vector<GlobalState*> _childs;
    vector<int> _probs;
    queue<Matching> _fifo;
    queue<int> _subjects;
    vector<int> _class ;
    int _countVisit;
    int _dist ;
    int _depth;
    
    void trim();      
    void createNodes();
    //void execute(int macId, int transId, Transition* transPtr);
    vector<GlobalState*> evaluate();
    // Invoke explore when an unexplored transition is push onto the queue
    void explore(int subject);
    void addTask(Transition tr, int subject);
    void execute(int transId, int subjectId);

    void init() ;
    bool active();
    Transition* getActive(int& macId, int& transId);
    bool setActive(int macId, int transId) ;
    void recordProb();
    

public:
    // Default constructor creates a global state with all its Fsm's set to initial state 0.
    GlobalState():_countVisit(1),_dist(0) { init(); }
    // This copy constructor is used to create childs, 
    // it will automatically increase the distance of childs
    GlobalState(const GlobalState& gs):_gStates(gs._gStates), _countVisit(1), 
        _dist(gs._dist+1), _fifo(gs._fifo), _depth(gs._depth) {}
    // This constructor is used to create a new GlobalState by specifying 
    // the state of its individual machines
    GlobalState(vector<int> stateVec):_gStates(stateVec), _countVisit(1),_dist(0) {}
    // This constructor should be called first. It will set the static member of GlobalState,
    // such as number of state machines, the pointers to machines, etc.
    GlobalState(const vector<Fsm*>& macs);
    
    GlobalState* getChild (size_t i) { return _childs[i]; }
    int getProb( size_t i ) const { return _probs[i] ; }
    int getProb() const { return _depth ; }
    //GlobalState* getGlobalState( vector<int> gs ) ;
    int getVisit() { return _countVisit ;}
    int getDistance() { return _dist;}
    const vector<int> getStateVec() const { return _gStates ;}
    size_t size() { return _childs.size() ; }
    bool hasChild() { return size()!=0; }
    bool isBusy() { return !_fifo.empty();}

    void findSucc();
    int increaseVisit(int inc) { return _countVisit += inc ; }        
    // For each child global states, update their distance from initial state
    // increase the step length from the initial global state for livelock detection
    void updateTrip();    

    string toString() ;

    static bool init(GlobalState*) ;
    static void clearAll() ;
};

class GlobalStateHashKey
{
public:
    GlobalStateHashKey(const GlobalState* gs):_gState(gs->getStateVec()), _depth(gs->getProb())
    {
        _sum = 0 ;
        for( size_t ii = 0 ; ii < _gState.size() ; ++ii ) 
            _sum += _gState[ii];
    }
    GlobalStateHashKey(const vector<int>& vec):_gState(vec)
    {
        _sum = 0 ;
        for( size_t ii = 0 ; ii < _gState.size() ; ++ii ) 
            _sum += _gState[ii];
    }

    size_t operator() () const { return (_sum); }
    bool operator == (const GlobalStateHashKey& k) 
    {
        assert(_gState.size() == k._gState.size() );
        for( size_t ii = 0 ; ii < _gState.size() ; ++ii ) 
            if( _gState[ii] != k._gState[ii] )
                return false ;

        if( _depth != k._depth )
            return false;
        return true;
    }
 
private:
    vector<int> _gState;
    GlobalState* _ptr;
    int _depth;

    int _sum;
};






#endif