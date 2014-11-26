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
vector<StateMachine*> GlobalState::_machines;
Service* GlobalState::_service = NULL;
GlobalState* GlobalState::_root = NULL;
set<GlobalState*> GlobalState::_all ;
Parser* GlobalState::_psrPtr = NULL;

GlobalState::GlobalState(GlobalState* gs)
: _visit(1), _dist(gs->_dist), _depth(gs->_depth), _white(true), _origin(gs->_origin)
{
    // Clone the pending tasks
    // Duplicate the container since the original gs->_fifo cannot be popped
    queue<MessageTuple*> cloneTasks = gs->_fifo ;
    while( !cloneTasks.empty() ) {
        MessageTuple* task = cloneTasks.front() ;
        
        _fifo.push(task->clone());
        cloneTasks.pop();
    }
    
    // Copy Snapshots
    _gStates.resize( gs->_gStates.size() );
    for( size_t m = 0 ; m < _machines.size() ; ++m ) {
        _gStates[m] = gs->_gStates[m]->clone();
    }
    // Copy CheckerState
    _checker = gs->_checker->clone() ;
    // Copy the ServiceSnapshot
    _srvcState = gs->_srvcState->clone();
    
#ifdef VERBOSE
    cout << "Create new GlobalState from " << this->toString() << endl;
#endif
#ifdef TRACE
    insertNode(this);
#endif
}

GlobalState::GlobalState(const GlobalState* gs)
    : _visit(1), _dist(gs->_dist), _depth(gs->_depth),
      _white(true), _origin(gs->_origin), trail_(gs->trail_) {
  // Copy Snapshots
  _gStates.resize( gs->_gStates.size() );
  for( size_t m = 0 ; m < _machines.size() ; ++m ) {
    _gStates[m] = gs->_gStates[m]->clone();
  }
  // Copy CheckerState
  _checker = gs->_checker->clone() ;
  // Copy the ServiceSnapshot
  _srvcState = gs->_srvcState->clone();
}

GlobalState::GlobalState(vector<StateMachine*> macs, CheckerState* chkState)
    :_visit(1), _dist(0), _white(true)
{
    if( _nMacs < 0 ) {
        _nMacs = (int)macs.size();
        _machines = macs ;      
    }
    else 
        assert( _nMacs == macs.size() );

    init();
    
    _service->reset();
    _srvcState = _service->curState();
    
    if( chkState == 0 ) {
        _checker = new CheckerState();
    }
    else {
        _checker = chkState->clone() ;
    }
#ifdef TRACE
    insertNode(this);
#endif
}

GlobalState::GlobalState(vector<StateSnapshot*>& stateVec)
: _visit(1),_dist(0), _white(true)
{
    assert(stateVec.size() == _machines.size());
    for( size_t i = 0 ; i < stateVec.size() ; ++i ) {
        _gStates[i] = stateVec[i]->clone() ;
    }
    
#ifdef TRACE
    insertNode(this);
#endif
}

GlobalState::~GlobalState()
{
    // Release the StateSnapshot's stored in _gStates
    for( size_t i = 0 ; i < _gStates.size() ; ++i ) {
        delete _gStates[i];
    }
    
    while( !_fifo.empty() ) {
        MessageTuple* task = _fifo.front() ;
        delete task ;
        _fifo.pop();
    }
    
    if (_srvcState)
        delete _srvcState;
#ifdef TRACE
    removeNode(this);
#endif
}

void GlobalState::init()
{
    _depth = 0;
    _gStates = vector<StateSnapshot*>(_nMacs);
    for( size_t ii = 0 ; ii < _machines.size() ; ++ii ) {
        _machines[ii]->reset();
        _gStates[ii] = _machines[ii]->curState();
    }
    /*
    _service->reset();
    _srvcState = _service->curState();
     */
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

GlobalState* GlobalState::getChild(size_t i)
{
    vector<GlobalState*>::iterator it = _childs.begin() ;
    for( size_t c = 0 ; c < i ; ++c )
        it++ ;
    return *it;
}

const vector<string> GlobalState::getStringVec() const
{
    vector<string> ret(_gStates.size());
    for( size_t m = 0 ; m < _gStates.size() ; ++m ) {
        ret[m] = _gStates[m]->toString();
    }
    return ret;
}

void GlobalState::findSucc()
{     
    try {
        vector<vector<MessageTuple*> > arrOutMsgs;
        vector<StateSnapshot*> statuses;
        bool high_prob;
        vector<bool> probs;
        // Execute each null input transition 
        for( size_t m = 0 ; m < _machines.size() ; ++m ) {
            arrOutMsgs.clear();
            statuses.clear();

            int idx = 0 ;
            // Restore the states of machines to current GlobalState
            restore();
            // Find the next null input transition of current GlobalState. The idx is
            // to be the next starting point
            vector<MessageTuple*> pendingTasks;
            do {
                restore();
                pendingTasks.clear();
                idx = _machines[m]->nullInputTrans(pendingTasks, high_prob, idx);
                if( idx < 0 ) {
                    // No null transition is found for machines[m]
                    break;
                }
                else {
                    // Null input transition is found. Store the matching                
                    // Create a clone of current global state
                    GlobalState* cc = new GlobalState(this);
                    // Execute state transition
                    cc->_gStates[m] = _machines[m]->curState();
                    cc->_srvcState = _service->curState();
                    // Push tasks to be evaluated onto the queue
                    cc->addTask( pendingTasks );
                    // Record the probability
                    if( !high_prob )
                        cc->_depth++;
                    // Store the newly created child GlobalState
                    _childs.push_back(cc);
                }
            } while( idx > 0 ) ;
        }

        // For each child just created, simulate the message reception of each
        // messsage (task) in the queue _fifo using the function evaluate()
        size_t cidx = 0 ;
        while( cidx < _childs.size() ) {
            try {
                vector<GlobalState*> ret = _childs[cidx]->evaluate();
                if( ret.size() > 1 ) {
                    for( size_t rii = 0 ; rii < ret.size() ; ++rii )
                        _childs.push_back(ret[rii]) ;
                    eraseChild(cidx) ;
                }
                else {
                    ++cidx ;
                }
            } catch (GlobalState* blocked) {
#ifdef TRACE
                // Print the sequence that leads to this unmatched transition
                vector<GlobalState*> seq;
                this->pathRoot(seq);
                printSeq(seq);
#endif
                // Remove the child that is blocked by unmatched transition
                MessageTuple *blockedMsg = _childs[cidx]->_fifo.front();
                string debug_dest = StateMachine::IntToMachine(blockedMsg->destId());
                string debug_destMsg = StateMachine::IntToMessage(blockedMsg->destMsgId());
                if (debug_dest != "") {
                    cout << debug_dest << endl;
                    cout << debug_destMsg << endl;
                }
                _childs.erase(_childs.begin()+cidx);
                // Continue on evaluating other children
                cout << "REMOVE global state. CONTINUE" << endl ;
                continue ;
            } catch (string str) {
                // This catch phrase should only be reached when no matching transition
                // found for a message reception. When such occurs, erase the child that
                // has no matching transition and print the error message. The process of
                // verification will not stop
                _childs.erase(_childs.begin()+cidx);
                cerr << "When finding successors (findSucc) " 
                     << this->toString() << endl ;
                cerr << str << endl ;
                
#ifdef TRACE_UNMATCHED
                throw this;
#endif
            }

        }
               
    } catch ( exception& e ) {
        cerr << e.what() << endl ;
    } catch (...) {
        cerr << "When finding successors (findSucc) " << this->toString() << endl ;
        throw;
    }
}

void GlobalState::findSucc(vector<GlobalState*>& successors) {
  findSucc();
  successors = _childs;
}

vector<GlobalState*> GlobalState::evaluate() 
{
#ifdef VERBOSE_EVAL
    cout << "Evaluating tasks in " << this->toString() << endl ;
#endif
    vector<GlobalState*> ret;
    while( !_fifo.empty() ) {
        // Get a task out of the queue
        MessageTuple* tuple = _fifo.front() ;
        _fifo.pop();
#ifdef VERBOSE_EVAL
        cout << "Evaluating task: " << tuple->toString() << endl;
#endif
        
        int idx = 0;
        bool high_prob ;
        int macNum = tuple->destId();
        vector<MessageTuple*> pending;
        
        if( tuple->destId() > 0 ) {
            // This label send message to another machine
#ifdef VERBOSE_EVAL
            cout << "Machine " << tuple->subjectId()
            << " sends message " << tuple->destMsgId()
            << " to machine " << tuple->destId() << endl ;
#endif
            this->restore();
            pending.clear();
            // The destined machine processes the tuple
            idx = _machines[macNum-1]->transit(tuple, pending, high_prob, 0);
            // Let the Service model process the MessageTuple
            if (_service)
                _service->putMsg(tuple);

            if( idx < 0 ) {
                // No found matching transition, this null transition should not be carry out
                // Another null input transition should be evaluate first
                stringstream ss ;
                ss << "No matching transition for message: " << endl
                   << msg2str(tuple) 
                   << "GlobalState = " << this->toString() << endl ;
                // Print all the task in _fifo
                ss << "Content in fifo: " << endl ;
                while( !_fifo.empty() ) {
                    MessageTuple* content = _fifo.front();
                    _fifo.pop();

                    ss << "Destination Machine ID = " << content->destId()
                        << " Message ID = " << content->destMsgId() << endl ;
                    delete content;
                }
                _fifo.push(tuple);
                    
#ifndef ALLOW_UNMATCHED
                delete tuple;
                throw ss.str();
#else
                cout << ss.str() ;
                cout << "SKIP unmatched transition. " << endl;                
                throw this;
#endif
            } // no matching transition found
            else {
                do {
                    // Create a new child
                    GlobalState* creation = new GlobalState(this);
                    // Take snapshot, add tasks to queue and update probability as
                    // before, but this time operate on the created child
                    delete creation->_gStates[macNum-1];
                    creation->_gStates[macNum-1] = _machines[macNum-1]->curState();
                    creation->_srvcState = _service->curState();
                    creation->addTask(pending);
                    if( !high_prob )
                        creation->_depth++;
                    ret.push_back(creation);                                                            
#ifdef VERBOSE_EVAL
                    queue<MessageTuple*> dupFifo = creation->_fifo ;
                    cout << "Pending tasks in the new child: " << endl ;
                    while( !dupFifo.empty() ) {
                        MessageTuple* tt = dupFifo.front() ;
                        dupFifo.pop() ;
                        cout << tt->toString() << endl ;
                    }
#endif
                    this->restore();
                    pending.clear();
                    idx = _machines[macNum-1]->transit(tuple, pending, high_prob, idx);
                    // Let the Service model process the MessageTuple
                    if (_service)
                        _service->putMsg(tuple);
                } while( idx > 0 );
            } // Matching transition(s) found
            
            if( ret.size() == 1 ) {
                // Only one matching transition found. There is no need to create new child
#ifdef VERBOSE_EVAL
                cout << "Only one matching transition found. " <<  endl ;
#endif                
                // Modify the state vector of the current globalState to be the same
                // as the only child
                GlobalState* only = ret.front() ;
                // Save the pending tasks and the states of the only child
                // (TODO: optimize this section)
                for( size_t m = 0 ; m < _gStates.size() ; ++m ) {
                    _gStates[m] = only->_gStates[m]->clone() ;
                }
                _srvcState = only->_srvcState->clone();
#ifdef VERBOSE_EVAL
                cout << "Move the pending tasks from the only child to its parent. " << endl;
                cout << "Remove task: " << endl;
#endif
                while( !_fifo.empty() ) {
                    // Clean up the task queue. Deallocate the task since these tasks will not
                    // be evaluated
#ifdef VERBOSE_EVAL
                    cout << _fifo.front()->toString() << endl;
#endif
                    delete _fifo.front();
                    _fifo.pop() ;
                }
#ifdef VERBOSE_EVAL
                cout << "from parent." << endl ;
                cout << "Move from the only child " << endl ;
#endif
                while( !only->_fifo.empty() ) {
                    // Clean up the task queue of the only child, but do not deallocate the
                    // task because these tasks will be evaluated later
#ifdef VERBOSE_EVAL
                    cout << only->_fifo.front()->toString() << endl ;
#endif
                    _fifo.push( only->_fifo.front() );
                    only->_fifo.pop() ;
                }
#ifdef VERBOSE_EVAL
                cout << "to parent. Delete the only child." << endl ;
#endif
                // Increment the depth if a low probability transition is encountered
                _depth = only->_depth ;
                
                // Replace the only element in the array to be returned by the original
                // GlobalState
                delete only;
                ret.clear();
            }
            else if( ret.size() > 1 ){
                // Multiple transitions: non-deterministic transition
                // Return the created childs right away
#ifdef VERBOSE_EVAL
                cout << "Multiple matching transitions found. " << endl;
#endif
                delete tuple;
                return ret;
            }
            else {
                // No matching transition is found.
                // Save current snapshot of the machine, and continue execution
                delete _gStates[macNum-1];
                _gStates[macNum-1] = _machines[macNum-1]->curState();
                delete _srvcState;
                _srvcState = _service->curState();
            }
        } // if destId >=0, messages being passed to other machines
        
        // Deallocate the message tuple
        delete tuple ;
    } // end while (task fifo)

    ret.clear();
    ret.push_back(this);
    return ret;
}

void GlobalState::addTask(vector<MessageTuple*> msgs)
{
#ifdef VERBOSE
    cout << "Tasks: " << endl;
#endif
    for( size_t iOuts = 0 ; iOuts < msgs.size() ; ++iOuts ) {     
        this->_fifo.push(msgs[iOuts]);
#ifdef VERBOSE
        cout << msgs[iOuts]->toString() << endl ;
#endif
    }

#ifdef VERBOSE
    cout << "is(are) added to the task FIFO queue." << endl ;
#endif
}

void GlobalState::updateTrip()
{
    vector<GlobalState*>::iterator it;
    for( it = _childs.begin() ; it != _childs.end() ; ++it ) {
        if( (*it)->_depth != this->_depth ) {
            assert((*it)->_depth > this->_depth ) ;
            (*it)->_dist = 0 ;
        }
        else if( (*it)->_dist < this->_dist + 1 ) {
            (*it)->_dist = this->_dist + 1 ;
        }
    }
}

void GlobalState::updateParents()
{
    vector<GlobalState*>::iterator it;
    for( it = _childs.begin() ; it != _childs.end() ; ++it ) {
        assert( (*it)->_parents.size() == 0 );
        (*it)->_parents.push_back(this);
    }
}

// This function removes the leafy GlobalState if it has no successor
// If removing such leaf also turns its parent a leaf, recursively remove its leafy parent
// Return true if its parent turned leafy and is removed;
// return false if only leaf is removed
bool GlobalState::removeBranch(GlobalState* leaf)
{
    if( leaf == _root )
        return false ;
    
    assert(leaf->_parents.size() == 1) ;
    
    GlobalState* par = leaf->_parents.front() ;
    vector<GlobalState*>::iterator it = par->_childs.begin() ;
    bool found = false;
    for( ; it != par->_childs.end() ; ++it ) {
        if( (*it) == leaf ) {
            par->_childs.erase(it);
            found = true ;
            break ;
        }
    }

    assert(found);
    
    bool ret = false;
    if( par->_childs.size() == 0 ) {
        removeBranch(par) ;
        ret = true ;
    }
    delete leaf ;
    return ret;
}

bool GlobalState::removeNode(GlobalState *gs)
{
    set<GlobalState*>::iterator it = _all.find(gs) ;
    if( it != _all.end()) {
        _all.erase(it) ;
        return true;
    }
    else
        return false;
}

bool GlobalState::insertNode(GlobalState *gs)
{
    _all.insert(gs);
    return true ;
}

void GlobalState::merge(GlobalState *gs)
{
    this->_visit += gs->_visit ;
    
    // Take care of the _childs vector of parents of gs
    for( size_t i = 0 ; i < gs->_parents.size() ; i++ ) {
        GlobalState* par = gs->_parents[i];
        vector<GlobalState*>::iterator it = par->_childs.begin() ;
        for( ; it != par->_childs.end() ; ++it ) {
            if( (*it) == gs ) {
                (*it) = this ;
                break ;
            }
        }
        assert( it != par->_childs.end() );
    }

    this->addParents(gs->_parents);
    
    delete gs;
}
   
bool GlobalState::init(GlobalState* s)
{
    _root = s;
    return true;
}

void GlobalState::addOrigin(GlobalState* rootStop)
{
    for( size_t i = 0 ; i < _origin.size() ; ++i ) {
        if( _origin[i]->toString() == rootStop->toString() ) {
            if( _origin[i]->_depth == rootStop->_depth )
                return ;
        }
    }
    _origin.push_back(rootStop);
}

void GlobalState::printOrigins(bool (*printStop)(GlobalState*, GlobalState*))
{
    for( size_t i = 0 ; i < _origin.size() ; ++i ) {
        int diffDepth = this->_depth - _origin[i]->_depth ;
        if( diffDepth <= 0 ) {
            if( diffDepth == 0 ) {
                if( printStop == 0 || printStop(this,_origin[i]) ) {
                    cout << _origin[i]->toString() << " =" << diffDepth << "=>"
                         << this->toString() << endl ;
#ifdef TRACE_STOPPING
                    cout << "Print the path between origin and current states:" << endl ;
                    vector<GlobalState*> seq;
                    this->pathRoot(seq, _origin[i]);
                    printSeq(seq);
#endif
                }
            }
            else {
                cout << "Tracing back to a stopping state with lower probability. ERROR"
                     << endl ;
            }
        }
    }
}

string GlobalState::toString() const
{
    stringstream ss ;
    ss << "[" ;
    for( size_t ii = 0 ; ii < _gStates.size()-1 ; ++ii )
        ss << _gStates[ii]->toString() << "," ;
    ss << _gStates.back()->toString() << "]" ;

    return ss.str();
}
/*
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
}*/

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

void GlobalState::eraseChild(size_t idx)
{
    assert(idx < _childs.size()) ;
    for( size_t ii = idx ; ii < _childs.size()-1 ; ++ii ) {
        _childs[ii] = _childs[ii+1] ;
    }
    _childs.pop_back() ;
}

void GlobalState::clearAll()
{
    /*
    GSHash::iterator it = _uniqueTable.begin();
    while( it != _uniqueTable.end() ) {
        delete (*it).second ;
        ++it;
    }
     */
}

void GlobalState::setService(Service* srvc)
{
    _service = srvc;
    //_srvcState = _service->curState();
}

bool rootStop(GlobalState* gsPtr)
{
    //if( gsPtr->_parents.size() == 0 )
    if( true)
        return true ;
    else 
        return false;
}
void GlobalState::pathRoot(vector<GlobalState* > &arr)
{
    pathRoot(arr, _root);
}

void GlobalState::pathRoot(vector<GlobalState* >& arr, const GlobalState* end)
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

        if( gs->toString() == end->toString() ) {
            // root found
            // Trace back
#ifdef VERBOSE
            cout << "Root found when tracing back. " << endl;
#endif
            do {
                arr.push_back(gs);
                gs = gs->getChild(gs->_trace) ;
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
            if( gs->_parents.size() == 0 )
                cout << "No parents." << endl ;
        } // if
    }
    GlobalState::resetColor();
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

        //if( gs == this ) {
        if( gs->toString() == this->toString() ) {
            // this: the globalstate where the BFS is started from
            // gs == this: the search return to the starting point, a cycle must be found
            // Trace back
            do {
                arr.push_back(gs);
                gs = gs->getChild(gs->_trace) ;
            } while( gs != this );
            arr.push_back(this);
        }
        else {
            for( size_t ii = 0 ; ii < gs->_parents.size() ; ++ii ) {
                GlobalState* par = gs->_parents[ii];
                if( par->_depth != gs->_depth )
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
                gs = gs->getChild(gs->_trace) ;
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
    set<GlobalState*>::iterator it = _all.begin() ;
    for( ; it != _all.end() ; ++ it )
        (*it)->_white = true;
    /*
    for( GSHash::iterator it = _uniqueTable.begin() ; it != _uniqueTable.end() ; ++it ) 
        (*it).second->_white = true;    
     */
}

size_t GlobalState::markPath(GlobalState* ptr)
{
    for( size_t ii = 0 ; ii < _childs.size() ; ++ii ) {
        if( getChild(ii) == ptr ) {
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

void GlobalState::printSeq(const vector<GlobalState*>& seq) {
  if (seq.size() < 1)
    return ;
  for (int ii = 0 ; ii < (int)seq.size()-1 ; ++ii) {
    if (seq[ii]->getProb() != seq[ii+1]->getProb())
      cout << seq[ii]->toString() << endl << "\n -p-> ";
    else
      cout << seq[ii]->toString() << endl << "\n -> ";
  }
  cout << seq.back()->toString() << endl;
}

string GlobalState::msg2str(MessageTuple *msg)
{
    stringstream ss;
    ss << "Source Machine = " << _psrPtr->IntToMachine(msg->subjectId()) << endl
       << "Destination Machine = " << _psrPtr->IntToMachine(msg->destId()) << endl
       << "Message = " << _psrPtr->IntToMessage(msg->destMsgId()) << endl ;
    return ss.str() ;
}

void GlobalState::restore()
{
    for( size_t m = 0 ; m < _machines.size() ; ++m )
        _machines[m]->restore(_gStates[m]);
    _service->restore(_srvcState);
}

void GlobalState::store()
{
    for( size_t m = 0 ; m < _machines.size(); ++m )
        _gStates[m] = _machines[m]->curState();
    _srvcState = _service->curState();
}

void GlobalState::printTrail() const {
  if (trail_.size()) {
    trail_.front()->printTrail();
    int p = getProb() - trail_.front()->getProb();
    cout << "-p" << p << "->" << endl;
  } else {
    assert(getProb() == 0);
  }
  for (const auto s : trail_) {
    cout << "->" << s->toString() << endl;
  }
}
