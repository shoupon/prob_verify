#include <vector>
#include <cassert>
#include <string>
#include <sstream>
#include <queue>

#include "globalstate.h"

#define VERBOSE

int GlobalState::_nMacs = -1;
vector<Fsm*> GlobalState::_machines;
GSHash GlobalState::_uniqueTable = GSHash(15) ;

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
        GlobalState* cc = new GlobalState(*this) ;        
        
        // Execute state transition
        cc->execute(vecTrans[ii].getId(), subjects[ii]);        
        
        // Push transition to be evaluated onto the queue
        cc->addTask(vecTrans[ii], subjects[ii]);    
        _childs.push_back(cc);
    }    

    for( size_t cIdx = 0 ; cIdx < _childs.size() ; ++cIdx ) {
        vector<GlobalState*> ret = _childs[cIdx]->evaluate();
        if( ret.size() > 1 ) {
            _childs.erase(_childs.begin()+cIdx);            
            cIdx--;
            _childs.insert(_childs.end(), ret.begin(), ret.end());
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
}

vector<GlobalState*> GlobalState::evaluate() 
{
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
            
            if( matched.size() == 1 ) {
                Transition tr = st->getTrans(matched[0].first);
                execute(matched[0].first, lbl.first);
                addTask(tr, lbl.first);
            }
            else if( matched.size() == 0 ) {
                continue;
            }
            else {
                // Multiple transitions: non-deterministic transition
                ret.resize(matched.size());
                for( size_t retIdx = 0 ; retIdx < ret.size() ; ++retIdx ) {
                    ret[retIdx] = new GlobalState(*this);
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
    for( size_t iOuts = 0 ; iOuts < tr.getNumOutLabels() ; ++iOuts ) {
        OutLabel lbl = tr.getOutLabel(iOuts);        
        this->_fifo.push(Matching(lbl,subject,tr.getId()));
    }
}

void GlobalState::updateTrip(int old)
{
    if( old + 1 < this->_dist )
        _dist = old + 1 ;
}
   
bool GlobalState::init(GlobalState* s) 
{
    return _uniqueTable.insert( GlobalStateHashKey(s->_gStates), s );
}


string GlobalState::toString() 
{
    stringstream ss ;
    for( size_t ii = 0 ; ii < _gStates.size() ; ++ii ) 
        ss << _gStates[ii] << " " ;

    return ss.str();
}

void GlobalState::trim()
{
    for( size_t ii = 0 ; ii < _childs.size() ; ++ii ) {
        // If the newly created global state is already in _uniqueTable, link the global state
        // If not, add such global state to _uniqueTable    
        GSHash::iterator it = _uniqueTable.find( GlobalStateHashKey(_childs[ii]) );
        if( it != _uniqueTable.end() ) {
            delete _childs[ii] ;
            _childs[ii] = (*it).second;        
            _childs[ii]->increaseVisit(this->_countVisit);                 
        }
        else {
            _uniqueTable.insert( GlobalStateHashKey(_childs[ii]), _childs[ii] ) ;
        }
    }
}

void GlobalState::execute(int transId, int subjectId)
{
    // Change state of one of the component machines
    State* curState = _machines[subjectId]->getState(_gStates[subjectId]) ;        
    // Execute state transition
    _gStates[subjectId] = curState->getNextState(transId)->getID();

    // Low probability transition
    if( !curState->getTrans(transId).isHigh() )
        _depth++;

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