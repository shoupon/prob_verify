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

GlobalState::GlobalState(GlobalState* gs)
    : _depth(gs->_depth), path_count_(gs->path_count_) {
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
}

GlobalState::GlobalState(const GlobalState* gs)
    : _depth(gs->_depth), trail_(gs->trail_),
      path_count_(gs->path_count_) {
  // Copy Snapshots
  for (auto ss : gs->_gStates)
    _gStates.push_back(ss->clone());
  // Copy CheckerState
  _checker = gs->_checker->clone() ;
  // Copy the ServiceSnapshot
  _srvcState = gs->_srvcState->clone();
}

GlobalState::GlobalState(vector<StateMachine*> macs, CheckerState* chkState)
    : path_count_(0) {
  if (_nMacs < 0) {
    _nMacs = (int)macs.size();
  }
  else {
    assert(_nMacs == macs.size());
  }
  _machines = macs;

    init();
    
    _service->reset();
    _srvcState = _service->curState();
    
    if( chkState == 0 ) {
        _checker = new CheckerState();
    }
    else {
        _checker = chkState->clone() ;
    }
}

GlobalState::GlobalState(vector<StateSnapshot*>& stateVec)
    : path_count_(0) {
  assert(stateVec.size() == _machines.size());
  for( size_t i = 0 ; i < stateVec.size() ; ++i ) {
      _gStates[i] = stateVec[i]->clone() ;
  }
}

GlobalState::~GlobalState() {
  // Release the StateSnapshot's stored in _gStates
  for (auto ss : _gStates)
    delete ss;
  while (!_fifo.empty()) {
    MessageTuple* task = _fifo.front() ;
    delete task ;
    _fifo.pop();
  }
  if (_srvcState)
    delete _srvcState;
  delete _checker;
}

void GlobalState::init()
{
    _depth = 0;
    //_gStates = vector<StateSnapshot*>(_nMacs);
    _gStates.clear();
    for (auto m_ptr : _machines) {
      m_ptr->reset();
      _gStates.push_back(m_ptr->curState());
    }
    /*
    for ( size_t ii = 0 ; ii < _machines.size() ; ++ii ) {
        _machines[ii]->reset();
        _gStates[ii] = _machines[ii]->curState();
    }
    */
    /*
    _service->reset();
    _srvcState = _service->curState();
     */
}       

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

size_t GlobalState::snapshotSize() const {
  size_t sum = 0;
  for (auto p : _gStates)
    sum += p->getBytes();
  return sum;
}

size_t GlobalState::getBytes() const {
  return sizeof(GlobalState) + snapshotSize();
}

void GlobalState::findSucc(vector<GlobalState*>& nd_choices) {
    try {
        vector<vector<MessageTuple*> > arrOutMsgs;
        vector<StateSnapshot*> statuses;
        int prob_level;
        vector<bool> probs;
        // Execute each null input transition 
        for (size_t m = 0; m < _machines.size(); ++m) {
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
                idx = _machines[m]->nullInputTrans(pendingTasks, prob_level, idx);
                if (idx < 0) {
                    // No null transition is found for machines[m]
                    break;
                }
                else {
                    // Null input transition is found. Store the matching
                    // Create a clone of current global state
                    GlobalState* cc = new GlobalState(this);
                    // Execute state transition
                    delete cc->_gStates[m];
                    cc->_gStates[m] = _machines[m]->curState();
                    delete cc->_srvcState;
                    cc->_srvcState = _service->curState();
                    // Push tasks to be evaluated onto the queue
                    cc->addTask( pendingTasks );
                    cc->_depth += prob_level;
                    nd_choices.push_back(cc);
                }
            } while( idx > 0 ) ;
        }

        // For each child just created, simulate the message reception of each
        // messsage (task) in the queue _fifo using the function evaluate()
        for (auto choice : nd_choices) {
            choice->blocked_ = false;
            try {
                if (choice->hasTasks())
                    choice->evaluate();
            } catch (GlobalState* blocked) {
              // Remove the child that is blocked by unmatched transition
              MessageTuple *blockedMsg = blocked->_fifo.front();
              string debug_dest = StateMachine::IntToMachine(blockedMsg->destId());
              string debug_destMsg = StateMachine::IntToMessage(
                  blockedMsg->destMsgId());
              if (debug_dest != "") {
                cout << debug_dest << endl;
                cout << debug_destMsg << endl;
              }
              choice->blocked_ = true;
              // Continue on evaluating other children
              cout << "REMOVE global state." << endl;
              cout << "CONTINUE exploring " << toReadableMachineName() << endl;
              continue ;
            } catch (string str) {
                // This catch phrase should only be reached when no matching transition
                // found for a message reception. When such occurs, erase the child that
                // has no matching transition and print the error message. The process of
                // verification will not stop
                choice->blocked_ = true;
                cerr << "When finding successors (findSucc) " 
                     << this->toString() << endl ;
                cerr << str << endl ;
                
#ifdef TRACE_UNMATCHED
                throw this;
#endif
            }

        } // for each choice in nd_choices

        // TODO(shoupon): collapse all probabilistic choices that follow after a
        // non-deterministic choice. Make sure the probability labels have been
        // assigned correctly
        for (auto choice : nd_choices) {
            if (choice->hasChild())
                choice->collapse(choice);
            else
                choice->_childs.push_back(new GlobalState(choice));
        }
    } catch (exception& e) {
        cerr << e.what() << endl ;
    } catch (...) {
        cerr << "When finding successors (findSucc) " << this->toString() << endl ;
        throw;
    }
}

void GlobalState::clearSucc() {
  for (auto c : _childs)
    delete c;
  _childs.clear();
}

// Recursively process the synchronizing messages created in previous
// non-deterministic choice or probabilistic choices
void GlobalState::evaluate() {
#ifdef VERBOSE_EVAL
    cout << "Evaluating tasks in " << this->toString() << endl ;
#endif
    vector<GlobalState*> ret;
    assert(!_fifo.empty());
    MessageTuple *tuple = _fifo.front();
    _fifo.pop();
#ifdef VERBOSE_EVAL
    cout << "Evaluating task: " << tuple->toString() << endl;
#endif

    int idx = 0;
    int macNum = tuple->destId();
    int prob_level;
    vector<MessageTuple*> pending;
    assert(tuple->destId() > 0);
#ifdef VERBOSE_EVAL
    cout << "Machine " << tuple->subjectId()
    << " sends message " << tuple->destMsgId()
    << " to machine " << tuple->destId() << endl ;
#endif
    restore();
    pending.clear();
    // The destined machine processes the tuple
    idx = _machines[macNum-1]->transit(tuple, pending, prob_level, idx);
    if (_service)
        _service->putMsg(tuple);
    if (idx < 0) {
        // No found matching transition, this non-deterministic choice
        // should not be evaluated. Continue to the next
        // non-deterministic choice.
        stringstream ss ;
        ss << "No matching transition for message: " << endl
           << tuple->toReadable()
           << "GlobalState = " << this->toReadable() << endl ;
        // Print all the task in _fifo
        ss << "Content in fifo: " << endl ;
        while (!_fifo.empty()) {
            MessageTuple* content = _fifo.front();
            _fifo.pop();

            ss << "Destination Machine ID = " << content->destId()
                << " Message ID = " << content->destMsgId() << endl ;
            delete content;
        }
        delete tuple;
#ifndef ALLOW_UNMATCHED
        throw ss.str();
#else
        cout << ss.str() ;
        cout << "SKIP unmatched transition. " << endl;                
        this->blocked_ = true;
        throw this;
#endif
    } // no matching transition found

    while (1) {
        GlobalState* creation = new GlobalState(this);
        // Take snapshot, add tasks to queue and update probability as
        // before, but this time operate on the created child
        delete creation->_gStates[macNum-1];
        creation->_gStates[macNum-1] = _machines[macNum-1]->curState();
        delete creation->_srvcState;
        creation->_srvcState = _service->curState();
        creation->addTask(pending);
        creation->_depth += prob_level;
        _childs.push_back(creation);
#ifdef VERBOSE_EVAL
        queue<MessageTuple*> dupFifo = creation->_fifo ;
        cout << "Pending tasks in the new child: " << endl ;
        while (!dupFifo.empty()) {
            MessageTuple* tt = dupFifo.front() ;
            dupFifo.pop() ;
            cout << tt->toString() << endl ;
        }
#endif

        // Continue to the next probabilistic choice
        restore();
        pending.clear();
        idx = _machines[macNum-1]->transit(tuple, pending, prob_level, idx);
        if (_service)
            _service->putMsg(tuple);
        if (idx < 0)
            break;
    }

    if (_fifo.size()) {
        for (auto c : _childs) {
            assert(c->hasTasks());
            c->evaluate();
        }
    }
}

void GlobalState::collapse(GlobalState *nd_root) {
  vector<GlobalState *> child_dup = _childs;
  _childs.clear();
  for (auto c : child_dup) {
    if (c->hasChild()) {
      // non-leaf
      c->collapse(nd_root);
      delete c;
    } else {
      // leaf
      nd_root->_childs.push_back(c);
    }
  }
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

bool GlobalState::init(GlobalState* s)
{
    _root = s;
    return true;
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

string GlobalState::toReadable() const {
  string ret;
  ret += "[";
  int k = 0;
  for (auto g : _gStates) {
    if (k++)
      ret += ",";
    ret += g->toReadable();
  }
  ret += "]";
  return ret;
}

string GlobalState::toReadableMachineName() const {
  string ret;
  ret += "[";
  int k = 0;
  int j = _gStates.size();
  for (auto g : _gStates) {
    ret += _machines[k++]->getName() + ": ";
    ret += g->toReadable();
    if (--j)
      ret += ",";
    ret += "\n";
  }
  ret += "]";
  return ret;
}

void GlobalState::eraseChild(size_t idx)
{
    assert(idx < _childs.size()) ;
    delete _childs[idx];
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

GlobalState* self;
bool selfStop(GlobalState* gsPtr)
{
    if( gsPtr == self )
        return true ;
    else 
        return false;
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
    ss << "Source Machine = " << StateMachine::IntToMachine(msg->subjectId())
       << endl
       << "Destination Machine = " << StateMachine::IntToMachine(msg->destId())
       << endl
       << "Message = " << StateMachine::IntToMessage(msg->destMsgId()) << endl;
    return ss.str() ;
}

void GlobalState::restore() {
  for (int m = 0 ; m < _machines.size() ; ++m)
    _machines[m]->restore(_gStates[m]);
  _service->restore(_srvcState);
}

void GlobalState::store()
{
    for( size_t m = 0 ; m < _machines.size(); ++m )
        _gStates[m] = _machines[m]->curState();
    _srvcState = _service->curState();
}

void GlobalState::mutateState(const StateSnapshot* snapshot, int mac_id) {
  delete _gStates[mac_id];
  _gStates[mac_id] = snapshot->clone();
  restore();
}

// returns head of the trail, that is, the entry state which is the entry point
// to this class
int GlobalState::printTrail(string (*translation)(int)) const {
  for (const auto s : trail_) {
    cout << "-> " << translation(s) << endl;
  }
  if (trail_.size()) {
    return trail_.front();
  } else {
    assert(getProb() == 0);
  }
  return -1;
}
