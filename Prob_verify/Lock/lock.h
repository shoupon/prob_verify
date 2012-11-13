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

class LockMessage;

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
    int _f;
    int _b;
    int _m;
    
    int _current;
    
    const int _range;
    
    MessageTuple* createResponse(string msg, string dst,
                                 MessageTuple* inMsg, int toward, int time );
    bool toDeny(MessageTuple* inMsg, vector<MessageTuple*>& outMsgs);
    bool toIgnore(MessageTuple* inMsg, vector<MessageTuple*>& outMsgs);
    bool toRelease(MessageTuple *inMsg, vector<MessageTuple *> &outMsgs) ;
    bool toTimeout(MessageTuple *inMsg, vector<MessageTuple *> &outMsgs);
    
    LockMessage* abortMsg();
};

class LockMessage : public MessageTuple
{
public:
    // Constructor: src, dest, srcMsg, destMsg, subject all retain the same implication as
    // is ancestor, MessageTuple;
    // k: the ID of the source lock
    // lock: the ID of the destination competitor. (exception: when the message is
    // "complete", which is to notify the controller that the lock is released, the field
    // is used to tell the controller which competitor this lock was associated to
    LockMessage(int src, int dest, int srcMsg, int destMsg, int subject, int k,
                int to, int t)
    :MessageTuple(src, dest, srcMsg, destMsg, subject), _k(k), _to(to), _t(t) {}
    
    LockMessage( const LockMessage& msg )
    :MessageTuple(msg._src, msg._dest, msg._srcMsg, msg._destMsg, msg._subject)
    , _k(msg._k), _to(msg._to), _t(msg._t) {}
    
    LockMessage(int src, int dest, int srcMsg, int destMsg, int subject,
                      const LockMessage& msg)
    :MessageTuple(src, dest, srcMsg, destMsg, subject)
    , _k(msg._k), _to(msg._to), _t(msg._t) {}
    
    ~LockMessage() {}
    
    size_t numParams() {return 3; }
    int getParam(size_t arg) { return (arg==2)?_t:((arg==1)?_to:_k); }
    
    string toString() ;
    
    LockMessage* clone() const ;
private:    
    const int _k ;
    const int _to ;
    const int _t;
};

class LockSnapshot : public StateSnapshot
{
public:
    friend class Lock;
    
    LockSnapshot(int ts, int front, int back, int master, int state)
    :_ss_ts(ts), _ss_f(front), _ss_b(back), _ss_m(master), _stateId(state) {}
    
    ~LockSnapshot() {} ;
    int curStateId() const { return _stateId; }
    // Returns the name of current state as specified in the input file
    string toString() ;
    // Cast the Snapshot into a integer. Used in HashTable
    int toInt() { return ( (_ss_f << 12) + (_ss_b << 8) + (_ss_m << 4 ) )*_ss_m
                         + _stateId ; }
    LockSnapshot* clone() const { return new LockSnapshot(_ss_ts, _ss_f,
                                                          _ss_b, _ss_m, _stateId); }
    
private:
    int _ss_ts;
    int _ss_f;
    int _ss_b;
    int _ss_m;
    
    int _stateId;
};



#endif
