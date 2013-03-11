#ifndef GLOBAL_H
#define GLOBAL_H

#include <vector>
#include <cassert>
#include <string>
#include <queue>

#include "fsm.h"
#include "state.h"
#include "statemachine.h"
#include "checker.h"
#include "myHash.h"
#include "parser.h"

class GlobalStateHashKey;
class GSVecHashKey;
class GlobalState;

typedef Hash<GlobalStateHashKey, GlobalState*> GSHash;
typedef Hash<GSVecHashKey, vector<string> > GSVecHash;

// A vector of int defines the states of machines in this global state using state number
// An integer represents the probability class of global state transition
typedef pair<vector<int>,int> GSProb;

/*
class Matching
{
public:
    OutLabel _outLabel ;
    int _source;
    int _transId;

    Matching(OutLabel lbl, int s, int transId):_outLabel(lbl), _source(s),_transId(transId) {}
};*/

class GlobalState
{
    // The ID's of states in a global state
    // A table stores all of the reachable globalState. Each reachable globalState is
    // unique. The _uniqueTable stores the pointers to each unique GlobalState
    static GlobalState* _root;
    static Parser* _psrPtr;
    
    vector<GlobalState*> _childs;
    vector<GlobalState*> _parents;
    queue<MessageTuple*> _fifo;

    int _countVisit;
    int _dist ;
    int _depth;
    
    // Used in breadth-first search
    bool _white;
    size_t _trace;
    
    // Used when determine transitions between stopping states
    vector<GlobalState*> _origin;
protected:
    // Number of machines
    static int _nMacs ;
    // array of ptr to machines
    static vector<StateMachine*> _machines;
    
    vector<StateSnapshot*> _gStates;
    CheckerState* _checker ;
private:
    //void trim();
    void addParents(const vector<GlobalState*>& arr);
    //void execute(int macId, int transId, Transition* transPtr);
    vector<GlobalState*> evaluate();
    // Invoke explore when an unexplored transition is push onto the queue
    void explore(int subject);
    void addTask(vector<MessageTuple*> msgs);
    
    void init() ;
    bool active();
    Transition* getActive(int& macId, int& transId);
    bool setActive(int macId, int transId) ;
    void recordProb();

    // Used in breadth-first search
    void resetColor();
    size_t markPath(GlobalState* ptr);
    //bool rootStop(GlobalState* gsPtr);
    //bool selfStop(GlobalState* gsPtr);
    
    static void printSeq(const vector<GlobalState*>& seq);
    
public:
    // Default constructor creates a global state with all its machine set to initial
    // state 0.
    GlobalState():_countVisit(1),_dist(0), _white(true) { init(); }
    // This copy constructor is used to create childs, 
    // it will automatically increase the distance of childs
    GlobalState(GlobalState* gs);
    // This constructor is used to create a new GlobalState by specifying 
    // the state of its individual machines
    GlobalState(vector<StateSnapshot*>& stateVec) ;
    // *** This constructor should be called first. It will set the static member of
    // GlobalState, such as number of state machines, the pointers to machines, etc.
    GlobalState(vector<StateMachine*> macs, CheckerState* chkState = 0);
    ~GlobalState();
    
    GlobalState* getChild (size_t i) { return _childs[i]; }
    int getProb() const { return _depth ; }
    int getVisit() { return _countVisit ;}
    int getDistance() { return _dist;}
    const vector<StateSnapshot*> getStateVec() const { return _gStates ;}
    CheckerState* chkState() { return _checker; }
    const vector<string> getStringVec() const;
    size_t size() { return _childs.size() ; }
    bool hasChild() { return size()!=0; }
    bool isBusy() { return !_fifo.empty();}

    void findSucc();
    int increaseVisit(int inc) { return _countVisit += inc ; }        
    // For each child global states, update their distance from initial state
    // increase the step length from the initial global state for livelock detection
    void updateTrip();
    void removeParents() { _parents.clear() ; }
    void updateParents() ;
    static bool removeBranch(GlobalState* leaf) ;
    void merge(GlobalState* gs);

    // Take the states of machines as stored in _gStates
    void restore();
    // Save the current state of machines to _gStates
    void store();

    // This function will start tracing back until root it found. The path will be saved in
    // arr. This is best used to print out the transitions from the initial state that lead
    // to deadlock
    void pathRoot(vector<GlobalState*>& arr);
    void pathRoot(vector<GlobalState*>& arr, const GlobalState* end);
    // This function will start tracing back until a cycle is found. Only high probability transitions
    // will be checked. 
    // If there is no such cycle that contains no low probability transition, return false;
    // If a cycle is found, return true and the found cycle is saved to arr. The first and the last
    // element in the vector<GlobalState*> is the same.
    void pathCycle(vector<GlobalState*>& arr);
    void BFS(vector<GlobalState*>& arr, bool (*stop)(GlobalState*));
    
    void addOrigin(GlobalState* rootStop);
    void printOrigins(bool (*printStop)(GlobalState*, GlobalState*) = 0);

    string toString() const ;

    void setRoot() { _root = this; }
    static bool init(GlobalState*) ;    
    static void clearAll() ;
    
    static void setParser(Parser* ptr) { _psrPtr = ptr ;}
};

class GlobalStateHashKey
{
public:
    GlobalStateHashKey(const GlobalState* gs)
        : _stateStr( gs->toString() )
    {
        vector<StateSnapshot*> mirror = gs->getStateVec() ;
        
        _sum = 0 ;
        for( size_t m = 0 ; m < mirror.size() ; ++m ) {
            _sum += mirror[m]->toInt() ;
        }
    }

    size_t operator() () const { return (_sum); }
    bool operator == (const GlobalStateHashKey& k) 
    {
        if( _stateStr != k._stateStr )
            return false ;

        return true;
    }
 
private:
    int _sum;
    string _stateStr;
};

class GSVecHashKey
{
public:
    GSVecHashKey(const vector<StateSnapshot*>& vec)
    {
        _sum = 0 ;
        _arrStr.resize(vec.size());
        for( size_t ii = 0 ; ii < vec.size() ; ++ii ) {
            _arrStr[ii] = vec[ii]->toString();
            _sum += vec[ii]->toInt() ;
        }
    }
    
    size_t operator() () const { return (_sum); }
    bool operator == (const GSVecHashKey& k)
    {
        assert(_arrStr.size() == k._arrStr.size() );
        for( size_t ii = 0 ; ii < _arrStr.size() ; ++ii ) {
            if( _arrStr[ii] != k._arrStr[ii] )
                return false ;
        }
        
        return true;
    }
    
private:
    vector<string> _arrStr;
    int _sum ;
};





#endif