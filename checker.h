//
//  checker.h
//  verification
//
//  Created by Shou-pon Lin on 3/11/13.
//  Copyright (c) 2013 Shou-pon Lin. All rights reserved.
//

#ifndef CHECKER_H
#define CHECKER_H

#include <iostream>
#include <vector>
using namespace std;

#include "statemachine.h"

class Checker;

class CheckerState
{
    friend Checker;
public:
    CheckerState() {}
    virtual CheckerState* clone()  { return new CheckerState();  }
};

class Checker
{
public:
    Checker() {}
    virtual bool check( CheckerState* cstate, const vector<StateSnapshot*>& mstate )
    { return true; }
    virtual CheckerState* initState() { return 0; }
};





#endif /* defined(CHECKER_H) */
