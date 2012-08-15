#include <vector>
#include <cassert>
#include <string>
#include <sstream>
#include <queue>
using namespace std;

#include "globalstate.h"
#include "define.h"

//#define VERBOSE

int GlobalState::_nMacs = -1;
vector<Fsm*> GlobalState::_machines;
GSHash GlobalState::_uniqueTable = GSHash(15) ;
GlobalState* GlobalState::_root = 0;

GlobalState::GlobalState(GlobalState* gs):_gStates(gs->_gStates), _countVisit(1), 
        _dist(gs->_dist+1), _fifo(gs->_fifo), _depth(gs->_depth) 
{ 
    _parents.push_back(gs);
#ifdef VERBOSE
    cout << "Create new GlobalState from " << this->toString();
#endif
}

GlobalState::GlobalState(const vector<StateMachine*>& macs)
    :_countVisit(1), _dist(0)
{
    if( _nMacs < 0 ) {
        _nMacs = macs.size();
        _machines = macs ;      
    }
    else 
        assert( _nMacs != macs.size() );    

    init();
}

void GlobalState::init()
{
    _depth = 0;
    _gStates = vector<int>(_nMacs);
    for( size_t ii = 0 ; ii < _machines.size() ; ++ii ) {
        _gStates[ii] = _machines[ii]->getInitState()->getID() ;
    } 
}       

/*
// Return if there's any activated transition in this global state
bool GlobalState::active()
{
    for( size_t m = 0 ; m < _machines.size() ; ++m ) {
        if( _actives[m].size() > 0 )
            return true ;
    }
    return false ;
}

Transition* GlobalState::getActive(int& macId, int& transId)
{
    macId = transId = -1;
    for( size_t m = 0 ; m < _machines.size() ; ++m ) {
        for( size_t t = 0 ; t < _actives[m].size() ; ++t ) {
            macId = m ;
            transId = _actives[m].back() ;
            _actives[m].pop_back();
            return &(_machines[m]->getState(_gStates[m])->getTrans(transId)) ;
        }
    }
    return 0;
}*/
/*
// TODO
void GlobalState::explore(int subject)
{    
    while( !_fifo.empty() ) {
        Matching toExec = _fifo.front();
        _fifo.pop();
        execute(subject, &toExec);  // CHANGE, this may return plural triggered edges
        // create childs if needed
    }
}*/

void GlobalState::findSucc()
{     
    try {
        /*vector<Transition> vecTrans;        
        vector<int> subjects;
        // Execute each null input transition and create a new global state for each transition
        for( size_t m = 0 ; m < _machines.size() ; ++m ) {
            State* st = _machines[m]->getState(_gStates[m]);
            for( size_t tt = 0 ; tt < st->getNumTrans() ; ++tt ) {           
                Transition tr = st->getTrans(tt) ;
                if( tr.getInputMessageId() == 0 ) {
                    // null input transition
                    //child->execute(m, tt, &tr);
                    vecTrans.push_back(tr);
                    subjects.push_back(m+1);
                } // if
            } // for
        } // for   */

        // Store the current state
        for( size_t m = 0 ; m < _machines.size() ; ++m ) {
            _gStates[m] = _machines[m]->curState();
        }

        size_t idx = 0 ;
        vector<vector<MessageTuple> > arrOutMsgs;
        vector<Snapshot> statuses;
        bool high_prob;
        vector<bool> probs;
        // Execute each null input transition 
        for( size_t m = 0 ; m < _machines.size() ; ++m ) {
            arrOutMsgs.clear();
            statuses.clear();

            while( idx >= 0 ) {
                // Restore the states of machines to current GlobalState
                restore();
                // Find the next null input transition of current GlobalState. The idx is increased 
                // to be the next starting point
                arrOutMsgs.push_back(vector<MessageTuple>() );
                idx = _machines[m]->nullInputTrans(arrOutMsgs.back(), high_prob, idx);  
                // Store the Snapshot of the machine after execute the null input transition
                statuses.push_back( _machines[m]->curState() ) ;
                // Store the probability of this transition
                probs.push_back(high_prob);                                
            }

            // Create a new global state for each transition
            for( size_t ii = 0 ; ii < statuses.size() ; ++ii ) {
                // Create a clone of current global state
                GlobalState* cc = new GlobalState(this);
                // Execute state transition
                cc->_gStates[m] = statuses[ii] ;
                // Push tasks to be evaluated onto the queue
                cc->addTask(arrOutMsgs[ii]);                
                // Record the probability
                if( !probs[ii] )
                    cc->_depth++;
                // Store the newly created child GlobalState
                _childs.push_back(cc);
            }
        }
              
        /*
        for( size_t ii = 0 ; ii < vecTrans.size() ; ++ii ) {
            // Create a clone of current global state
            GlobalState* cc = new GlobalState(this) ;     
            //cc->_childs.clear();
            
            // Execute state transition
            cc->execute(vecTrans[ii].getId(), subjects[ii]);        
            
            // Push transition to be evaluated onto the queue
            cc->addTask(vecTrans[ii], subjects[ii]);    
            _childs.push_back(cc);
        } */   

        for( size_t cIdx = 0 ; cIdx < _childs.size() ; ++cIdx ) {
            try {
                vector<GlobalState*> ret = _childs[cIdx]->evaluate();
                if( ret.size() > 1 ) {
                    _childs.erase(_childs.begin()+cIdx);
                    cIdx--;
                    _childs.insert(_childs.end(), ret.begin(), ret.end());
                }                
            } catch (string str) {
                _childs.erase(_childs.begin()+cIdx);                
                cerr << "When finding successors (findSucc) " 
                     << this->toString() << endl ;
                cerr << str << endl ;
                cIdx--;
            }

        }                    
        //_fifo.push(vecTrans[0]);
        //explore();
               
        recordProb();
        // Trim the list of childs    
        trim() ;
        // For each child global states, update their distance from initial state
        //updateTrip();
        // Create new object of found successors that are not recorded in the _uniqueTable
        // Also, increase the step length from the initial global state for livelock detection
        //createNodes();  
    } catch (...) {
        cerr << "When finding successors (findSucc) " << this->toString() << endl ;
        throw;
    }
}

vector<GlobalState*> GlobalState::evaluate() 
{
#ifdef VERBOSE
    cout << "Evaluating tasks in " << this->toString() << endl ;
#endif
    vector<MessageTuple> matched;
    vector<GlobalState*> ret;
    while( !_fifo.empty() ) {
        // Get a task out of the queue
        MessageTuple tuple = _fifo.front() ;
        _fifo.pop();   
        
        size_t idx = 0;
        bool high_prob ;
        int macNum = tuple.destId();
        if( tuple.destId() >= 0 ) {
            // This label send message to another machine           
            idx = _machines[macNum-1]->transit(tuple, matched, high_prob, idx);
#ifdef VERBOSE
            cout << "Machine " << tuple.subject() << " sends message " << tuple.destMsg()
                 << " to machine " << tuple.destId() << endl ;
#endif 
            if( matched.size() == 0 ) {
                // No found matching transition, this null transition should not be carry out
                // Another null input transition should be evaluate first
                stringstream ss ;
                ss << "No matching transition for outlabel: " 
                   << "Destination Machine ID = " << tuple.destId()
                   << " Message ID = " << tuple.destMsgId() << endl
                   << "GlobalState = " << this->toString() << endl ;   

                // Print all the task in _fifo
                ss << "Content in fifo: " << endl ;
                while( !_fifo.empty() ) {
                    tuple = _fifo.front();                    
                    _fifo.pop();

                    ss << "Destination Machine ID = " << tuple.destId()
                        << " Message ID = " << tuple.destMsgId() << endl ;
                }
                    
#ifndef ALLOW_UNMATCHED
                throw ss.str();
#else
                cout << ss.str() ;
                cout << "SKIP unmatched transition. CONTINUE" << endl;
#endif
            } // no matching transition found
            else if( idx < 0 ) {
                // Only one matching transition found. There is no need to create new child
#ifdef VERBOSE
                cout << "Only one matching transition found. " <<  endl ;
#endif
                // First, take a snapshot of current machine states
                _gStates[macNum] = _machines[macNum]->curState() ;
                // Add the matched tasks
                addTask(matched);
                // Increment the depth if a low probability transition is encountered
                if( !high_prob )
                    _depth++;                
            }
            else {
                // Multiple transitions: non-deterministic transition
                // Need to create more childs

                ret.clear();
                // Create a new child
                GlobalState* creation = new GlobalState(this);
                // Take snapshot, add tasks to queue and update probability as before, but
                // this time operate on the created child
                creation->_gStates[macNum] = _machines[macNum]->curState();
                creation->addTask(matched);
                creation->_parents = this->_parents;
                if( !high_prob )
                    creation->_depth++;
                ret.push_back(creation);

                while( idx >= 0 ) {
                    // Undo previous action
                    this->restore();
                    idx = _machines[macNum]->transit(tuple, matched, high_prob, idx);
                    // Create a new child
                    GlobalState* creation = new GlobalState(this);
                    // Take snapshot, add tasks to queue and update probability as before, but
                    // this time operate on the created child
                    creation->_gStates[macNum] = _machines[macNum]->curState();
                    creation->addTask(matched);
                    creation->_parents = this->_parents;
                    if( !high_prob )
                        creation->_depth++;
                    ret.push_back(creation);
                }

                return ret;
            } // end multiple-transition
        } // if destId >=0, messages being passed to other machines
    } // end while (task fifo)

    ret.push_back(this);
    return ret;
}

void GlobalState::addTask(vector<MessageTuple> msgs)
{   
#ifdef VERBOSE
    cout << "Tasks: " << endl;
#endif
    for( size_t iOuts = 0 ; iOuts < msgs.size() ; ++iOuts ) {     
        this->_fifo.push(msgs[iOuts]);
#ifdef VERBOSE
        cout << msgs[iOuts].destId() << "!" << msgs[iOuts].destMsgId() << endl ;
#endif
    }

#ifdef VERBOSE
    cout << "is(are) added to the task FIFO queue." << endl ;
#endif
}

void GlobalState::updateTrip()
{
    for( size_t ii = 0 ; ii < _childs.size() ; ++ii ) {
        if( _childs[ii]->_dist < this->_dist + 1 )
            _childs[ii]->_dist = this->_dist + 1 ;
    }
}
   
bool GlobalState::init(GlobalState* s) 
{
    _root = s;
    return _uniqueTable.insert( GlobalStateHashKey(s->_gStates), s );
}


string GlobalState::toString() 
{
    stringstream ss ;
    ss << "(" ;
    for( size_t ii = 0 ; ii < _gStates.size() ; ++ii ) 
        ss << _gStates[ii] << "," ;
    ss << "\b)" ;

    return ss.str();
}

void GlobalState::trim()
{
    for( size_t ii = 0 ; ii < _childs.size() ; ++ii ) {
        // If the newly created global state is already in _uniqueTable, link the global state
        // If not, add such global state to _uniqueTable    
        GSHash::iterator it = _uniqueTable.find( GlobalStateHashKey(_childs[ii]) );
        
        if( it != _uniqueTable.end() ) {
            // Such GlobalState with the same state vector and of the same class 
            // has already been created
            GlobalState* dup = _childs[ii];
            
            _childs[ii] = (*it).second;        
            _childs[ii]->increaseVisit(this->_countVisit);             
            _childs[ii]->addParents(dup->_parents);
            
            delete dup;
        }
        else {
            _uniqueTable.insert( GlobalStateHashKey(_childs[ii]), _childs[ii] ) ;
        }
    }
}
void GlobalState::addParents(const vector<GlobalState*>& arr)
{
    for( size_t ii = 0 ; ii < arr.size(); ++ii ) {
        bool exist = false ;
        for( size_t jj = 0 ; jj < this->_parents.size() ; ++jj ) {
            if( _parents[jj] == arr[ii] )
                exist = true;
        }

        if( !exist ) {
            _parents.push_back(arr[ii]);
        }
    }
}


void GlobalState::execute(int transId, int subjectId)
{
    // Change state of one of the component machines
    State* curState = _machines[subjectId-1]->getState(_gStates[subjectId-1]) ;        

#ifdef VERBOSE
    string origin = this->toString();
    cout << "Machine " << subjectId 
         << "executes transition " << curState->getTrans(transId).toString() << endl ;
#endif 

    // Execute state transition
    _gStates[subjectId-1] = curState->getNextState(transId)->getID();

    // Low probability transition
    if( !curState->getTrans(transId).isHigh() )
        _depth++;

#ifdef VERBOSE
    cout << origin << "->" << this->toString() << endl ;
#endif

}

void GlobalState::clearAll()
{
    GSHash::iterator it = _uniqueTable.begin();
    while( it != _uniqueTable.end() ) {
        delete (*it).second ;
        ++it;
    }
}

void GlobalState::recordProb()
{
    _probs.clear();
    for( size_t i = 0 ; i < _childs.size() ; ++i ) {
        _probs.push_back(_childs[i]->_depth);
    }
}


bool rootStop(GlobalState* gsPtr)
{
    //if( gsPtr->_parents.size() == 0 )
    if( true)
        return true ;
    else 
        return false;
}

void GlobalState::pathRoot(vector<GlobalState*>& arr)
{
    resetColor();
    arr.clear();

    // Reverse breadth-first search until the root is found
    queue<GlobalState*> unexplored;
    unexplored.push(this);
    
    _trace = 0;
    _trace--;

    while( !unexplored.empty() ) {
        GlobalState* gs = unexplored.front() ;
        unexplored.pop();
        gs->_white = false; // Paint the node black

        if( gs->_parents.size() == 0 ) {
            // root found
            // Trace back
            do {
                arr.push_back(gs);
                gs = gs->_childs[gs->_trace] ;
            } while( gs != this );
            arr.push_back(this);
            break;
        }
        else {
            for( size_t ii = 0 ; ii < gs->_parents.size() ; ++ii ) {
                GlobalState* par = gs->_parents[ii];
                if( par->_white ) {
                    par->markPath(gs);
                    unexplored.push(par);
                }
            }
        } // if
    }
    //BFS(arr, &rootStop);
}

GlobalState* self;
bool selfStop(GlobalState* gsPtr)
{
    if( gsPtr == self )
        return true ;
    else 
        return false;
}

void GlobalState::pathCycle(vector<GlobalState*>& arr)
{
    //self = this;
    //BFS(arr, &selfStop);
    resetColor();
    arr.clear();

    // Reverse breadth-first search until the root is found
    queue<GlobalState*> unexplored;
    for( size_t ii = 0 ; ii < _parents.size() ; ++ii ) { 
        if( _parents[ii]->_depth != _depth )
            continue;
        _parents[ii]->markPath(this);
        unexplored.push(_parents[ii]);
    }
    _trace = 0;
    _trace--;

    while( !unexplored.empty() ) {
        GlobalState* gs = unexplored.front() ;
        unexplored.pop();
        gs->_white = false; // Paint the node black

        if( this == _root ) {
            // root found
            // Trace back
            do {
                arr.push_back(gs);
                gs = gs->_childs[gs->_trace] ;
            } while( gs != this );
            arr.push_back(this);
        }
        else {
            for( size_t ii = 0 ; ii < gs->_parents.size() ; ++ii ) {
                GlobalState* par = gs->_parents[ii];
                if( par->_depth != _depth )
                    continue;
                if( par->_white ) {
                    par->markPath(gs);
                    unexplored.push(par);
                }
            }
        } // if
    }
}


void GlobalState::BFS(vector<GlobalState*>& arr, bool (*stop)(GlobalState*)) 
{
    resetColor();
    arr.clear();

    // Reverse breadth-first search until the root is found
    queue<GlobalState*> unexplored;
    for( size_t ii = 0 ; ii < _parents.size() ; ++ii ) {      
        _parents[ii]->markPath(this);
        unexplored.push(_parents[ii]);
    }
    _trace = 0;
    _trace--;

    while( !unexplored.empty() ) {
        GlobalState* gs = unexplored.front() ;
        unexplored.pop();
        gs->_white = false; // Paint the node black

        if( (*stop)(gs) ) {
            // root found
            // Trace back
            do {
                arr.push_back(gs);
                gs = gs->_childs[gs->_trace] ;
            } while( gs != this );
        }
        else {
            for( size_t ii = 0 ; ii < gs->_parents.size() ; ++ii ) {
                GlobalState* par = gs->_parents[ii];
                if( par->_white ) {
                    par->markPath(gs);
                    unexplored.push(par);
                }
            }
        } // if
    }
}


void GlobalState::resetColor()
{
    for( GSHash::iterator it = _uniqueTable.begin() ; it != _uniqueTable.end() ; ++it ) 
        (*it).second->_white = true;    
}

size_t GlobalState::markPath(GlobalState* ptr)
{
    for( size_t ii = 0 ; ii < _childs.size() ; ++ii ) {
        if( _childs[ii] == ptr ) {
            _trace = ii;
            return _trace;
        }
    }

    stringstream ss ;
    ss << "Child " << ptr->toString() 
       << " in GlobalState " << this->toString() ;
    throw runtime_error(ss.str());

    size_t ret = 0 ;
    ret--;
    return ret;
}

void GlobalState::restore()
{
    for( size_t m = 0 ; m < _machines.size() ; ++m )
        _machines[m]->restore(_gStates[m]);
}

void GlobalState::store()
{
    for( size_t m = 0 ; m < _machines.size(); ++m )
        _gStates[m] = _machines[m]->curState();
}