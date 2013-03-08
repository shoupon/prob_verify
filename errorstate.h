//
//  errorstate.h
//  Prob_verify
//
//  Created by Shou-pon Lin on 9/18/12.
//  Copyright (c) 2012 Shou-pon Lin. All rights reserved.
//

#ifndef ERRORSTATE_H
#define ERRORSTATE_H

#include <vector>
using namespace std;

#include "globalstate.h"
#include "statemachine.h"

class ErrorState : public GlobalState
{
    vector<int> _checks;
public:
    ErrorState(GlobalState* gs)
    :GlobalState(gs) {}

    void addCheck(StateSnapshot* criterion, int mac);
    bool match(const GlobalState* gs);
};

#endif
