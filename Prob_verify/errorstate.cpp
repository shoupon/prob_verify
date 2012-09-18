//
//  errorstate.cpp
//  Prob_verify
//
//  Created by Shou-pon Lin on 9/18/12.
//  Copyright (c) 2012 Shou-pon Lin. All rights reserved.
//

#include "errorstate.h"
#include "statemachine.h"

void ErrorState::addCheck(StateSnapshot* criterion, int mac)
{
    if( mac >= _nMacs || mac < 0)
        return ;
    
    _gStates[mac] = criterion->clone() ;
    _checks.push_back(mac);
}

bool ErrorState::match(const GlobalState* gs)
{
    vector<StateSnapshot*> vecSnap = gs->getStateVec() ;
    
    // For each machine that needs to be checked. All the checked machine should be in the
    // error state specified in ErrorState object
    for( size_t i = 0 ; i < _checks.size() ;++i ) {
        int mac = _checks[i] ;
        // check if the machine is in the error state
        if( vecSnap[mac]->toString() != _gStates[mac]->toString() )
            return false;  // If it isn't in the error state, then it doesn't match error
    }
    // All the machines that need to be checked are in the error state
    return true;
}