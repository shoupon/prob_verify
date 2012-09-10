//
//  competitor.h
//  Prob_verify
//
//  Created by Shou-pon Lin on 8/24/12.
//  Copyright (c) 2012 Shou-pon Lin. All rights reserved.
//

#ifndef COMPETITOR_H
#define COMPETITOR_H

#include "../statemachine.h"
#include "lock_utils.h"

class CompetitorMessage ;

class Competitor : public StateMachine
{
public:
    // Constructor.
    // k: Identifier of the lock
    // delta: the duration of the lock
    // num: total number of vehicles
    Competitor(int k, int delta, int num, Lookup* msg, Lookup* mac) ;
    int transit(MessageTuple* inMsg, vector<MessageTuple*>& outMsgs,
                bool& high_prob, int startIdx = 0);
    int nullInputTrans(vector<MessageTuple*>& outMsgs, bool& high_prob, int startIdx = 0);
    void restore(const StateSnapshot* snapshot) ;
    StateSnapshot* curState() ;
    void reset() ;
    
private:
    const int _id;
    const int _delta;
    string _name;
    int _machineId;
    
    int _t;
    int _front;
    int _back;
    
    int _current;
    
    const int _range;
    
    MessageTuple* createResponse(string msg, string dst, MessageTuple* inMsg, int toLock);
    MessageTuple* createReq(string dst, MessageTuple* inMsg, int toLock) ;
    
    string getFrontCh() {return Lock_Utils::getChannelName( _id, _front);}
    string getBackCh() { return Lock_Utils::getChannelName( _id, _back); }
    
    bool toRelease(MessageTuple *inMsg, vector<MessageTuple *> &outMsgs) ;
    bool toTimeout(MessageTuple *inMsg, vector<MessageTuple *> &outMsgs);

    CompetitorMessage* abortMsg();
};

class CompetitorMessage : public MessageTuple
{
public:
    // Constructor: src, dest, srcMsg, destMsg, subject all retain the same implication as
    // is ancestor, MessageTuple;
    // k: the ID of the source competitor
    // lock: the ID of the destination lock. Exception: -1 when the destination is the
    // controller
    CompetitorMessage(int src, int dest, int srcMsg, int destMsg, int subject
                      , int k, int lock)
    :MessageTuple(src, dest, srcMsg, destMsg, subject), _k(k),_lock(lock) { _nParams = 2;}
    
    CompetitorMessage( const CompetitorMessage& msg )
    :MessageTuple(msg._src, msg._dest, msg._srcMsg, msg._destMsg, msg._subject)
    , _k(msg._k), _lock(msg._lock), _t(msg._t), _nParams(msg._nParams) {}
    
    CompetitorMessage(int src, int dest, int srcMsg, int destMsg, int subject,
                      const CompetitorMessage& msg)
    :MessageTuple(src, dest, srcMsg, destMsg, subject),
    _k(msg._k), _lock(msg._lock), _t(msg._t), _nParams(msg._nParams) {}
    
    ~CompetitorMessage() {}
    

    
    CompetitorMessage* createReq(int timeGen);
    
    size_t numParams() { return _nParams; }
    int getParam(size_t arg) { return (arg==2)?_t:((arg==1)?_lock:_k) ; }
    
    string toString();
    
    CompetitorMessage* clone() const ;
    
private:
    // The ID of the competitor from which this message is generated
    const int _k;
    // The ID of the lock to which this message is destined
    const int _lock;
    int _t;
    int _nParams;
};

class CompetitorSnapshot : public StateSnapshot
{
public:
    friend class Competitor ;
    
    CompetitorSnapshot(int t, int front, int back, int state)
    :_ss_t(t), _ss_f(front), _ss_b(back), _stateId(state) {}
    
    ~CompetitorSnapshot() {} ;
    int curStateId() const { return _stateId; }
    // Returns the name of current state as specified in the input file
    string toString() ;
    // Cast the Snapshot into a integer. Used in HashTable
    int toInt() { return (_ss_t << 16 ) + (_ss_f << 8) + (_ss_b << 4 ) + _stateId ; }
    CompetitorSnapshot* clone() const
    { return new CompetitorSnapshot(_ss_t, _ss_f, _ss_b, _stateId); }
    
private:
    int _ss_t;
    int _ss_f;
    int _ss_b;
    
    int _stateId;
};



#endif
