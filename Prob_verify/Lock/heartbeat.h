//
//  heartbeat.h
//  Prob_verify
//
//  Created by Shou-pon Lin on 10/16/12.
//  Copyright (c) 2012 Shou-pon Lin. All rights reserved.
//

#ifndef HEARTBEAT_H
#define HEARTBEAT_H

#include <iostream>
using namespace std;

#include "define.h"
#include "lookup.h"
#include "../statemachine.h"


class HBSnapshot;
class HBMessage;
class Heartbeater;

class HBSnapshot: public StateSnapshot
{
    friend class Heartbeater;
    
    vector<int> _ss_beating;
public:
    HBSnapshot(vector<int> beating): _ss_beating(beating) {}
    ~HBSnapshot() {} ;
    int curStateId() const ;
    // Returns the name of current state as specified in the input file. Used to identify states
    // in the set STATET, STATETABLE, RS
    string toString() ;
    // Cast the Snapshot into a integer. Used in HashTable
    int toInt();
    HBSnapshot* clone() const { return new HBSnapshot(_ss_beating); }
};


class Heartbeater: public StateMachine
{
    const int _num;
    vector<int> _beating;
    
    const int _machineId;
public:
    Heartbeater( int numLock, Lookup* msg, Lookup* mac )
    : StateMachine(msg,mac), _num(numLock), _machineId(machineToInt("heartbeater"))
    { _beating.resize(_num,0);}
    
    ~Heartbeater() {}
    
    int transit(MessageTuple* inMsg, vector<MessageTuple*>& outMsgs,
                bool& high_prob, int startIdx = 0) ;
    int nullInputTrans(vector<MessageTuple*>& outMsgs,
                       bool& high_prob, int startIdx = 0) ;
    void restore(const StateSnapshot* snapshot) ;
    StateSnapshot* curState() { return new HBSnapshot(_beating);}
    void reset() { _beating.clear() ; _beating.resize(_num,0);  }
};

class HBMessage: public MessageTuple
{
public:
    HBMessage(int src, int dest, int srcMsg, int destMsg, int subject)
        : MessageTuple(src, dest, srcMsg, destMsg, subject) {}
    HBMessage( const MessageTuple& tuple ): MessageTuple(tuple) {}
    HBMessage* clone() const {return new HBMessage(*this); }
};





#endif // defined HEARTBEAT_H
