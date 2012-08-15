#include <string>
#include <vector>
using namespace std;

#include "define.h"

#include "fsm.h"

Fsm::Fsm(string name, Parser* ptr)
:_name(name), _current(0), StateMachine(ptr)
{
}


bool Fsm::addState(string name)
{
    int id = (int)_stateNames.size();
    InsertResult ir = _stateNames.insert( Entry(name, id) );
    if( ir.second == false ) 
        return false ;
    
    State* newState = new State(name, id) ;
    _states.push_back(newState) ;
    return true;
}

bool Fsm::addTransition(string from, string to, Transition& label)
{
    int idFrom = _stateNames.find(from)->second;
    int idTo = _stateNames.find(to)->second;

    return addTransition(idFrom, idTo, label);
}

bool Fsm::addTransition(int from, int to, Transition& label)
{
    // need to check if there's duplicate input label
    // There can be at most one high probability transition with a specific label for a state,
    // while there can be multiple low probability transitions with the same label
    if( _states[from]->findTransition(label) ) {
        // There is already such input label for _states[from]
        return false ;
    }
    _states[from]->addOutEdge(label, _states[to]);
    return true;
}

State* Fsm::getState(string name)
{
    int key = _stateNames.find(name)->second;
    return _states[key];
}

State* Fsm::getState(int id)
{
    if( id >= 0 && id < (int)_states.size() )
        return _states[id];
    else
        return 0 ;
}

void Fsm::reset()
{
    for( size_t ii = 0 ; ii < _states.size() ; ++ii ) {
        _states[ii]->reset();
    }
}

size_t Fsm::nullInputTrans(vector<MessageTuple>& outMsgs, bool& high_prob, size_t startIdx )
{
    State* cs = _states[_current];
    outMsgs.clear();
    
    // Go through all the transition of current state
    for( size_t tid = startIdx ; tid < cs->getNumTrans() ; ++tid ) {
        Transition tr = cs->getTrans((int)tid);
        if( tr.getFromMachineId() == 0 ) {
            // For each output label. There may be multiple output destined for different machines.
            for( size_t oid = 0 ; oid < tr.getNumOutLabels() ; ++oid ) {
                OutLabel lbl = tr.getOutLabel(oid);
                FsmMessage out(0,0, lbl.first, lbl.second, _psrPtr->machineToInt(this->_name) );
                outMsgs.push_back(out);
            }
            
            if( tr.isHigh() )
                high_prob = true;
            else
                high_prob = false;
            
            return tid+1;
        }
    }
    
    // The function will get to this line only when no null input transition is found
    return -1;
}
                




