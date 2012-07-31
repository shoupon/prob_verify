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

GlobalState::GlobalState(const vector<Fsm*>& macs)
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
        vector<Transition> vecTrans;
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
                    subjects.push_back(m);
                } // if
            } // for
        } // for   

              
        for( size_t ii = 0 ; ii < vecTrans.size() ; ++ii ) {
            // Create a clone of current global state
            GlobalState* cc = new GlobalState(this) ;     
            //cc->_childs.clear();
            
            // Execute state transition
            cc->execute(vecTrans[ii].getId(), subjects[ii]);        
            
            // Push transition to be evaluated onto the queue
            cc->addTask(vecTrans[ii], subjects[ii]);    
            _childs.push_back(cc);
        }    

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
    vector<Arrow> matched;
    vector<GlobalState*> ret;
    while( !_fifo.empty() ) {
        // Get a task out of the queue
        Matching match = _fifo.front();
        OutLabel lbl = match._outLabel;
        _fifo.pop();   
        

        if( lbl.first >= 0 ) {
            // This label send message to another machine
            State* st = _machines[lbl.first]->getState(_gStates[lbl.first]);
            st->receive(match._source, lbl.second, matched);
#ifdef VERBOSE
            cout << "Machine " << match._source << " sends message " << lbl.second 
                 << " to machine " << lbl.first << endl ;
#endif 
            
            if( matched.size() == 1 ) {
#ifdef VERBOSE
                cout << "Only one matching transition found. " <<  endl ;
#endif
                Transition tr = st->getTrans(matched[0].first);
                execute(matched[0].first, lbl.first);
                addTask(tr, lbl.first);
            }
            else if( matched.size() == 0 ) { 
                // No found matching transition, this null transition should not be carry out
                // Another null input transition should be evaluate first
                stringstream ss ;
                ss << "No matching transition for outlabel: " 
                   << "Destination Machine ID = " << lbl.first 
                   << " Message ID = " << lbl.second << endl
                   << "GlobalState = " << this->toString() << endl ;   

                // Print all the task in _fifo
                ss << "Content in fifo: " << endl ;
                while( !_fifo.empty() ) {
                    match = _fifo.front();
                    lbl = match._outLabel;
                    _fifo.pop();

                    ss << "Destination Machine ID = " << lbl.first 
                       << " Message ID = " << lbl.second << endl ;
                }
                    
#ifndef ALLOW_UNMATCHED
                throw ss.str();
#else
                cout << ss.str() ;
                cout << "SKIP unmatched transition. CONTINUE" << endl;
#endif
            }
            else {
                // Multiple transitions: non-deterministic transition
#ifdef VERBOSE
                cout << "Multiple transitions found matching" << endl ;
#endif
                ret.resize(matched.size());
                for( size_t retIdx = 0 ; retIdx < ret.size() ; ++retIdx ) {
                    ret[retIdx] = new GlobalState(this);
                    ret[retIdx]->_parents = this->_parents;

                    Transition tr = st->getTrans(matched[retIdx].first);
                    ret[retIdx]->execute(matched[retIdx].first, lbl.first);
                    ret[retIdx]->addTask(tr, lbl.first);
                }
                return ret;
            }
        }
    }

    ret.push_back(this);
    return ret;


}

void GlobalState::addTask(Transition tr, int subject)
{   
#ifdef VERBOSE
    cout << "Tasks: " << endl;
#endif
    for( size_t iOuts = 0 ; iOuts < tr.getNumOutLabels() ; ++iOuts ) {
        OutLabel lbl = tr.getOutLabel(iOuts);        
        this->_fifo.push(Matching(lbl,subject,tr.getId()));
#ifdef VERBOSE
        cout << lbl.first << "!" << lbl.second << endl ;
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
    State* curState = _machines[subjectId]->getState(_gStates[subjectId]) ;        

#ifdef VERBOSE
    string origin = this->toString();
    cout << "Machine " << subjectId 
         << "executes transition " << curState->getTrans(transId).toString() << endl ;
#endif 

    // Execute state transition
    _gStates[subjectId] = curState->getNextState(transId)->getID();

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