#include <vector>
#include <cassert>
#include <string>
#include <sstream>
#include <queue>

#include "globalstate.h"

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
    _gStates = vector<int>(_nMacs);
    for( size_t ii = 0 ; ii < _machines.size() ; ++ii ) {
        _gStates[ii] = _machines[ii]->getInitState()->getID() ;
    } 
    _actives = vector<vector<int>>(_nMacs) ; 
}       

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
}

void GlobalState::findSucc()
{
    Transition* transPtr ;
    bool ac = false ;
    int macId, transId ;
    while( transPtr = getActive(macId, transId) ) {
        // If there are active transitions,
        // execute each active transition and create a new global state for each transition
        ac = true ;
        
        execute(macId, transId, transPtr);        
    }

    if( !ac ) {
        // Else, execute each null input transition and create a new global state for each transition
        for( size_t m = 0 ; m < _machines.size() ; ++m ) {
            State* st = _machines[m]->getState(_gStates[m]);
            for( size_t tt = 0 ; tt < st->getNumTrans() ; ++tt ) {           
                Transition tr = st->getTrans(tt) ;
                if( tr.getInputMessageId() == 0 ) {
                    // null input transition
                    execute(m, tt, &tr);
                    //for( size_t nOut = 0 ; nOut < tr.getNumOutLabels() ; ++nOut )
                        
                }
            }
        }                
    }

    
    
    // Trim the list of childs    
    //trim() ;
    // For each child global states, update their distance from initial state
    //updateTrip();
    // Create new object of found successors that are not recorded in the _uniqueTable
    // Also, increase the step length from the initial global state for livelock detection
    //createNodes();    
}

void GlobalState::execute(int macId, int transId, Transition* transPtr)
{
    vector<Arrow> matched; // Arrow = pair<int, State*>
    GlobalState* child = new GlobalState();  
    for( size_t ii = 0 ; ii < transPtr->getNumOutLabels() ; ++ii ) {
        // Go through all the out labels
        OutLabel lb = transPtr->getOutLabel(ii);            
        if( lb.first >= 0 ) {
            bool set = child->setActive(lb.first, lb.second);
            // If an edge of a state is already activated, then repeat activation of an edge
            // results in undefined behavior. Modification to protocol specification is 
            // suggested
            if( !set ) {
                cout << "Repeat activation of an edge" << endl ;
                // TODO: print out the sequence that causes repeat activation
            }
            /*
            State* st = _machines[lb.first]->getState(_gStates[lb.first]);
            st->receive(macId, lb.second, matched);
          
            for( size_t a = 0 ; a < matched.size() ; ++a ) {
                Transition tr = st->getTrans(matched[a].first);
                for( size_t nLbl = 0 ; nLbl < tr.getNumOutLabels() ; ++nLbl ) {
                    OutLabel lbl = tr.getOutLabel(nLbl);
                    // setActive( toMachineId, messageId ):
                    // send message 'messageId' to machine 'toMachineId'
                    if( lbl.first >= 0 ) {
                        bool set = child->setActive(lbl.first, lbl.second);
                        // If an edge of a state is already activated, then repeat activation of an edge
                        // results in undefined behavior. Modification to protocol specification is 
                        // suggested
                        if( !set ) {
                            cout << "Repeat activation of an edge" << endl ;
                            // TODO: print out the sequence that causes repeat activation
                        }
                    }

                }
            }*/
        } // if
    }   

    int nextID = _machines[macId]->getState(_gStates[macId])->getNextState(transId)->getID() ;
    child->_gStates = this->_gStates;
    child->_gStates[macId] = nextID;

    // If the newly created global state is already in _uniqueTable, link the global state
    // If not, add such global state to _uniqueTable    
    GSHash::iterator it = _uniqueTable.find( GlobalStateHashKey(child) );
    if( it != _uniqueTable.end() ) {
        delete child ;
        child = (*it).second;        
        child->increaseVisit(this->_countVisit);                 
    }
    else {
        _uniqueTable.insert( GlobalStateHashKey(child), child ) ;
    }

    int prob = 0;
    if( !transPtr->isHigh() )
        prob = 1;

    child->updateTrip(this->_dist);

    _childs.push_back( GSProb(child->_gStates, prob) );
}
/*
void GlobalState::updateTrip()
{
    for( size_t ii = 0 ; ii < _childs.size() ; ++ii )
        if( _dist +1 < _childs[ii]->_dist )
            _childs[ii]->_dist = old + 1 ;  
}*/

void GlobalState::updateTrip(int old)
{
    if( old + 1 < this->_dist )
        _dist = old + 1 ;
}


   
/*
void GlobalState::trim() 
{
    // A table used to detect duplicate child global states created during findSucc()
    GSVecHash tab ;

    // Check if there are multiple identical global state. Remove the duplicate.
    // Assign entries of GlobalState* to child list
    for( size_t ii = 0 ; ii < _childs.size() ; ++ii ) {
        if( !tab.insert( GlobalStateHashKey(_childs[ii].first), _childs[ii].first ) ) {
            // If false is returned, which indicates entry is already created, then
            // assign the created entry to _childs[ii]
            GSVecHash::iterator it = tab.find(GlobalStateHashKey(_childs[ii].first));
            _childs[ii].first = (*it).second  ;            
        }
    } 
}*/
     
bool GlobalState::init(GlobalState* s) 
{
    return _uniqueTable.insert( GlobalStateHashKey(s->_gStates), s );
}

        
GlobalState* GlobalState::getGlobalState( vector<int> gs ) 
{   
    GSHash::iterator it = _uniqueTable.find(GlobalStateHashKey(gs)); 
    if( it == _uniqueTable.end() )
        return 0 ;
    else
        return (*it).second ;
}

string GlobalState::toString() 
{
    stringstream ss ;
    for( size_t ii = 0 ; ii < _gStates.size() ; ++ii ) 
        ss << _gStates[ii] << " " ;

    return ss.str();
}

void GlobalState::createNodes()
{
    GSHash::iterator it;
    for( size_t ii = 0 ; ii < _childs.size() ; ++ii ) {
        it = _uniqueTable.find(_childs[ii].first) ;
        if( it == _uniqueTable.end() ) {
            GlobalState* ptr = new GlobalState();
            ptr->_dist = this->_dist + 1 ;
            _uniqueTable.insert( GlobalStateHashKey(_childs[ii].first), ptr ) ;
        }
        else {
            GlobalState* ptr = (*it).second ;
            if( ptr->_dist < this->_dist + 1 )
                ptr->_dist = this->_dist +1 ;
        }
    }
}

bool GlobalState::setActive(int macId, int transId) 
{ 
    if( _actives[macId].size() > 0 )
        return false ;
    _actives[macId].push_back(transId); 
    return true;
} 