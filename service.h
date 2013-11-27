//
//  service.h
//  verification-lock
//
//  Created by Shou-pon Lin on 11/19/13.
//  Copyright (c) 2013 Shou-pon Lin. All rights reserved.
//

#ifndef SERVICE_H
#define SERVICE_H

#include <iostream>
#include <set>
using namespace std;

#include "statemachine.h"
class Service;

class ServiceSnapshot: public StateSnapshot {
    friend class Service;
public:
    ServiceSnapshot(int state):StateSnapshot(state) {}
    ServiceSnapshot* clone() const { return new ServiceSnapshot(_ss_state); }
};

class Service: public StateMachine {
public:
    Service(): StateMachine() {}
    ~Service();
    virtual int transit(MessageTuple* inMsg, vector<MessageTuple*>& outMsgs,
                bool& high_prob, int startIdx = 0) { return 3; }
    int nullInputTrans(vector<MessageTuple*>& outMsgs,
                       bool& high_prob, int startIdx = 0) { return -1; }
    virtual void restore(const ServiceSnapshot* snapshot) {
        _state = snapshot->_ss_state;
    }
    virtual ServiceSnapshot* curState() { return new ServiceSnapshot(_state);}

    void addInterface(MessageTuple *input) {_interface.insert(input->clone());}
    virtual void putMsg(MessageTuple *msg);
    
protected:
    class lessThan { // simple comparison function
    public:
        inline bool operator()(const MessageTuple* lhs, const MessageTuple* rhs)
        { return *lhs < *rhs; } // returns x>y
    };
    
    typedef set<MessageTuple *, lessThan> MsgSet;
    MsgSet _interface;
    
    virtual bool isMonitored(MessageTuple* msg);
};



#endif /* defined(SERVICE_H) */
