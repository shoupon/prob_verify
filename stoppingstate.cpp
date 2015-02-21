//
//  stoppingstate.cpp
//  Prob_verify
//
//  Created by Shou-pon Lin on 9/19/12.
//  Copyright (c) 2012 Shou-pon Lin. All rights reserved.
//

#include "stoppingstate.h"
#include "statemachine.h"

StoppingState::~StoppingState()
{
    for( size_t i = 0 ; i < _aSets.size() ; ++i )
        delete _aSets[i];
    for( size_t i = 0 ; i < _pSets.size() ; ++i )
        delete _pSets[i];
}

void StoppingState::addAllow(StateSnapshot* criterion, int mac)
{
    if( mac >= _nMacs || mac < 0)
        return ;
    
    _aSets.push_back( criterion->clone() );
    _allows.push_back(mac);
    assert(_aSets.size() == _allows.size());
    cout << "Adding criterion of machine " << mac << endl;
}

void StoppingState::addAllow(const StateSnapshot* criterion,
                             const string& machine_name) {
  int mac_id = StateMachine::machineToInt(machine_name);
  _aSets.push_back(criterion->clone());
  _allows.push_back(mac_id - 1);
  assert(_aSets.size() == _allows.size());
  cout << "Adding criterion of machine " << mac_id;
}

void StoppingState::addProhibit(StateSnapshot* criterion, int mac)
{
    if( mac >= _nMacs || mac < 0)
        return ;
    
    _pSets.push_back( criterion->clone() ) ;
    _prohibits.push_back(mac);
}

bool StoppingState::match(const GlobalState* gs)
{
    // For each machine that needs to be checked. All the checked machine should be in the
    // error state specified in ErrorState object
    
    vector<StateSnapshot*> vecSnap = gs->getStateVec() ;
    for( size_t i = 0 ; i < _prohibits.size() ;++i ) {
        int mac = _prohibits[i] ;
        // check if the machine is in the prohibited state
        if( vecSnap[mac]->toString() == _pSets[i]->toString() )
            return false;  // If it isn't in the error state, then it doesn't match error
    }
    
    for( size_t i = 0 ; i < _allows.size() ; ++i ) {
        int mac = _allows[i];
        // check if the machine is in the specified set
        if( !vecSnap[mac]->match(_aSets[i]) )
            return false ;
    }
    // All the machines that need to be checked are in the error state
    return true;
}

string StoppingState::toString()
{
    stringstream ss ;
    ss << "StoppingState: " << this << endl ;
    ss << "allows:" << endl ;
    for( size_t i = 0 ; i < _allows.size() ; ++i ) {
        ss << "Machine " << _allows[i] << ": " << _aSets[i]->toString() << endl ;
    }
    ss << "prohibits: " << endl ;
    for( size_t i = 0 ; i < _prohibits.size() ; ++i ) {
        ss << "Machine " << _prohibits[i] << ": " << _pSets[i]->toString() << endl ;
    }
    return ss.str() ;
}
