//
//  stoppingstate.h
//  Prob_verify
//
//  Created by Shou-pon Lin on 9/19/12.
//  Copyright (c) 2012 Shou-pon Lin. All rights reserved.
//

#ifndef STOPPING_H
#define STOPPING_H

#include <iostream>
#include <vector>
#include <string>
using namespace std;

#include "globalstate.h"
#include "statemachine.h"

class StoppingState : public GlobalState
{
    vector<int> _allows;
    vector<StateSnapshot*> _aSets;
    vector<int> _prohibits;
    vector<StateSnapshot*> _pSets;
public:
    StoppingState(GlobalState* gs):GlobalState(gs) {}
    virtual ~StoppingState();
    
    void addAllow(StateSnapshot* criterion, int mac);
    void addProhibit(StateSnapshot* criterion, int mac);
    virtual bool match(const GlobalState* gs);
    
    virtual string toString() ;
};


#endif
