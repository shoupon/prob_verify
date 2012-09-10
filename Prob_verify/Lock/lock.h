//
//  lock.h
//  Prob_verify
//
//  Created by Shou-pon Lin on 8/24/12.
//  Copyright (c) 2012 Shou-pon Lin. All rights reserved.
//

#ifndef LOCK_H
#define LOCK_H

#include "../statemachine.h"
#include "lock_utils.h"

class Lock : public StateMachine
{
public:
    // Constructor.
    // k: Identifier of the lock
    // delta: the duration of the lock
    // num: total number of vehicles
    Lock(int k, int delta, int num, Lookup* msg, Lookup* mac) ;
    
    int transit(MessageTuple* inMsg, vector<MessageTuple*>& outMsgs,
                        bool& high_prob, int startIdx = 0);
    int nullInputTrans(vector<MessageTuple*>& outMsgs,
                               bool& high_prob, int startIdx = 0);
    void restore(const StateSnapshot* snapshot) ;
    StateSnapshot* curState() ;
    void reset() ;
    
private:
    const int _id;
    const int _delta;
    string _name;
    int _machineId;
    
    int _ts;
    int _t2;
    int _old;
    int _new;
    
    int _current;
    
    const int _range;
    
    MessageTuple* createResponse(string msg, string dst, MessageTuple* inMsg);
};

class LockMessage : public MessageTuple
{
public:
    LockMessage(int src, int dest, int srcMsg, int destMsg, int subject, int k)
    :MessageTuple(src, dest, srcMsg, destMsg, subject), _k(k) {}
    
    LockMessage( const LockMessage& msg )
    :MessageTuple(msg._src, msg._dest, msg._srcMsg, msg._destMsg, msg._subject)
    , _k(msg._k) {}
    
    LockMessage(int src, int dest, int srcMsg, int destMsg, int subject,
                      const LockMessage& msg)
    :MessageTuple(src, dest, srcMsg, destMsg, subject)
    , _k(msg._k) {}
    
    ~LockMessage() {}
    
    size_t numParams() {return 0; }
    int getParam(size_t arg) { return _k; }
    
    string toString() ;
    
    LockMessage* clone() const ;
private:
    const int _k ;
        
};

class LockSnapshot : public StateSnapshot
{
public:
    friend class Lock;
    
    LockSnapshot(int ts, int t2, int oldCom, int newCom, int state)
    :_ss_ts(ts), _ss_t2(t2),_ss_old(oldCom), _ss_new(newCom), _stateId(state) {}
    
    ~LockSnapshot() {} ;
    int curStateId() const { return _stateId; }
    // Returns the name of current state as specified in the input file
    string toString() ;
    // Cast the Snapshot into a integer. Used in HashTable
    int toInt() { return (_ss_ts << 16 ) + (_ss_old << 8) + (_ss_new << 4 ) + _stateId ; }
    LockSnapshot* clone() const { return new LockSnapshot(_ss_ts, _ss_t2,
                                                          _ss_old, _ss_new, _stateId); }
    
private:
    int _ss_ts;
    int _ss_t2;
    int _ss_old;
    int _ss_new;
    
    int _stateId;
};



#endif
