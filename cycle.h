//
//  cycle.h
//  verification
//
//  Created by Shou-pon Lin on 4/29/13.
//  Copyright (c) 2013 Shou-pon Lin. All rights reserved.
//
#ifndef CYCLE_H
#define CYCLE_H

#include <vector>
#include <iostream>
using namespace std;

#include "sync.h"
#include "PUChecker.h"

class SyncSnapshot;


class Cycle: public Sync
{
public:
    Cycle( int numDeadline, Lookup* msg, Lookup* mac ) ;
    ~Cycle() { }
    
    int transit(MessageTuple* inMsg, vector<MessageTuple*>& outMsgs,
                bool& high_prob, int startIdx);
    int nullInputTrans(vector<MessageTuple*>& outMsgs,
                       bool& high_prob, int startIdx ) ;
    //void restore(const StateSnapshot* snapshot);
    //StateSnapshot* curState() ;
    // Reset the machine to initial state
    void reset() ;
};


#endif

