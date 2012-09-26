//
//  lock.cpp
//  Prob_verify
//
//  Created by Shou-pon Lin on 8/24/12.
//  Copyright (c) 2012 Shou-pon Lin. All rights reserved.
//
#include <sstream>
#include <string>
using namespace std;

#include "lock.h"
#include "competitor.h"

Lock::Lock(int k, int delta, int num, Lookup* msg, Lookup* mac)
:_id(k), _delta(delta), _range(num), StateMachine(msg,mac)
{
    // The name of the lock is "lock(i)", where i is the id of the machine
    _name = Lock_Utils::getLockName(_id);
    _machineId = machineToInt(_name);
    reset();
}


int Lock::transit(MessageTuple* inMsg, vector<MessageTuple*>& outMsgs,
                  bool& high_prob, int startIdx )
{
    outMsgs.clear();

    if( startIdx != 0 )
        return -1 ;
        
    high_prob = true ;
    
    string msg = IntToMessage(inMsg->destMsgId() ) ;
    switch ( _current ) {
        case 0 :
            if( msg == "REQUEST" ) {
                assert( typeid(*inMsg) == typeid(CompetitorMessage) );
                // Assignments
                int i = inMsg->getParam(0) ;
                int t = inMsg->getParam(2) ;
                _ts = t ;
                _old = i;
                // Response
                if( _old != _id ) {
                    MessageTuple* response = createResponse("LOCKED", "channel",
                                                            inMsg, _old, _ts);
                    outMsgs.push_back(response);
                }
                // Change State
                _current = 1;
                
                return 3;
            }
            else if( msg == "status" ) {
                assert( typeid(*inMsg) == typeid(CompetitorMessage)) ;
                // Response
                if(  inMsg->getParam(0) == _id ) {
                    string ownComp = Lock_Utils::getCompetitorName(_id) ;
                    MessageTuple* response = createResponse("free", ownComp,
                                                            inMsg, _id, -1);
                    outMsgs.push_back(response);
                    
                    return 3;
                }
            }
            else if( msg == "RELEASE" || msg == "timeout" ) {
                // Do nothing but return 3
                return 3;
            }
            break;
        
        case 1:
            if( msg == "RELEASE" ) {
                if( inMsg->getParam(0) == _old && inMsg->getParam(2) == _ts) {
                    // Response
                    MessageTuple* ctrlRes = new LockMessage(0, machineToInt("controller"),
                                                            0, messageToInt("complete"),
                                                            _machineId, _id, _old, -1);
                    outMsgs.push_back(ctrlRes);
                    // Change state
                    _current = 0;
                    reset();
                    
                    return 3;
                }
                else {
                    // The RELEASE is not intended for 
                    return 3;
                }
            }
            else if( msg == "status" ) {
                assert( typeid(*inMsg) == typeid(CompetitorMessage)) ;
                // Response
                if(  inMsg->getParam(0) == _id ) {
                    string ownComp = Lock_Utils::getCompetitorName(_id) ;
                    MessageTuple* response = createResponse("secured", ownComp,
                                                            inMsg, _id, -1);
                    outMsgs.push_back(response);
                    
                    return 3;
                }
            }
            else if( msg == "REQUEST" ) {
                int j = inMsg->getParam(0) ;
                if( j == _old ) {
                    int newTs = inMsg->getParam(2) ;
                    if( newTs < _ts ) {
                        // this is not always satisfied when the messages are not delivered
                        // in the order of their transmission 
                        // throw logic_error("Newly arrival REQUEST has smaller timestamp");
                        // ignore the message instead
                        return 3;
                    }
                    
                    // Assignments
                    // update ts
                    _ts = newTs ;
                    
                    // Response
                    if( _old != _id ) {
                        MessageTuple* response = createResponse("LOCKED", "channel",
                                                                inMsg, _old, _ts);
                        outMsgs.push_back(response);
                    }                            
                    return 3 ;
                }
                else {
                    // Assignments
                    _t2 = inMsg->getParam(2);
                    _new = j ;
                    
                    if( _t2 < _ts ) {
                        // True
                        // Response
                        MessageTuple* react = createResponse("INQUIRE", "channel",
                                                             inMsg, _old, _ts );
                        outMsgs.push_back(react);
                        
                        // Change state
                        _current = 2;
                    }
                    else {
                        // False
                        // Response
                        MessageTuple* response = createResponse("FAILED", "channel",
                                                                inMsg, _new, _t2 );
                        outMsgs.push_back(response);
                        MessageTuple* ctrlAbt =
                            new LockMessage(0, machineToInt("controller"),
                                            0, messageToInt("abort"),
                                            _machineId, _id, _new, -1);
                        outMsgs.push_back(ctrlAbt);
                        // Assign variables
                        _new = _t2 = -1;
                        // Change state
                        _current = 1;
                    }
                    
                    return 3;
                }
            }
            else if( msg == "timeout" ) {
                assert( inMsg->subjectId() == machineToInt("controller") ) ;
                int time = inMsg->getParam(0);
                if( time == _ts ) {
                    // Change state
                    _current = 0;
                    reset();
                }
                else {
                    // Ignore the timeout message, since this lock is no longer locked
                    // by another master
                }
                return 3;
            }
            
        case 2:
            if( msg == "ENGAGED" ) {
                if( inMsg->getParam(0) == _old && inMsg->getParam(2) == _ts) {
                    // Respond
                    MessageTuple* react = createResponse("FAILED", "channel",
                                                         inMsg, _new, _t2) ;
                    outMsgs.push_back(react);
                    MessageTuple* ctrlAbt =
                        new LockMessage(0, machineToInt("controller"),
                                        0, messageToInt("abort"),
                                        _machineId, _id, _new, -1);
                    outMsgs.push_back(ctrlAbt);
                    // Assign variables
                    _new = _t2 = -1;
                    // Change state
                    _current = 1;
                    
                    return 3;
                }
            }
            else if( msg == "RELEASE" ) {
                if( inMsg->getParam(0) == _old && inMsg->getParam(2) == _ts) {
                    // Respond
                    MessageTuple* react = createResponse("LOCKED", "channel",
                                                         inMsg, _new, _t2);
                    outMsgs.push_back(react);
                    MessageTuple* ctrlAbt =
                        new LockMessage(0, machineToInt("controller"),
                                        0, messageToInt("abort"),
                                        _machineId, _id, _old, -1);
                    outMsgs.push_back(ctrlAbt);
                    // Assignments
                    _ts = _t2;
                    _old = _new ;
                    _new = _t2 = -1;
                    // Change state
                    _current = 1;
                    
                    return 3;
                }
            }
            else if( msg == "status" ) {
                assert( typeid(*inMsg) == typeid(CompetitorMessage)) ;
                // Response
                if(  inMsg->getParam(0) == _id ) {
                    string ownComp = Lock_Utils::getCompetitorName(_id) ;
                    MessageTuple* response = createResponse("secured", ownComp,
                                                            inMsg, _id, -1);
                    outMsgs.push_back(response);
                    
                    return 3;
                }
            }
            else if( msg == "timeout" ) {
                assert( inMsg->subjectId() == machineToInt("controller") ) ;
                int time = inMsg->getParam(0);
                if( time == _ts ) {
                    // Same reaction as that of receiving RELEASE from old competitor
                    // Respond
                    MessageTuple* react = createResponse("LOCKED", "channel",
                                                         inMsg, _new, _t2);
                    outMsgs.push_back(react);
                    // Assignments
                    _ts = _t2;
                    _old = _new ;
                    _new = _t2 = -1;
                    // Change state
                    _current = 1;
                }
                else {
                    // Ignore the timeout message, since this lock is no longer locked
                    // by another master
                }
                
                
                return 3;
            }
        default:
            return -1;
            break;
    }
    
    return -1;
}

int Lock::nullInputTrans(vector<MessageTuple*>& outMsgs,
                   bool& high_prob, int startIdx)
{
    outMsgs.clear();
    return -1;
}

void Lock::restore(const StateSnapshot* snapshot)
{
    assert( typeid(*snapshot) == typeid(LockSnapshot) ) ;
    
    const LockSnapshot* sslock = dynamic_cast<const LockSnapshot*>(snapshot);
    _ts = sslock->_ss_ts;
    _t2 = sslock->_ss_t2;
    _old = sslock->_ss_old;
    _new = sslock->_ss_new;
    _current = sslock->_stateId;
}

StateSnapshot* Lock::curState()
{
    return new LockSnapshot(_ts, _t2, _old, _new, _current);
}


void Lock::reset()
{
    _ts = -1 ;
    _t2 = -1 ;
    _old = -1 ;
    _new = -1 ;
    _current = 0 ;
}

MessageTuple* Lock::createResponse(string msg, string dst, MessageTuple* inMsg,
                                   int toComp, int time)
{
    int outMsgId = messageToInt(msg);
    int dstId = machineToInt(dst);
    
    assert(inMsg->destId() == _machineId);
    assert(toComp < _range) ;
    
    MessageTuple* ret = new LockMessage(inMsg->subjectId(), dstId,
                                        inMsg->destMsgId(), outMsgId,
                                        _machineId, _id, toComp, time);
    return ret;
}

LockMessage* LockMessage::clone() const
{
    return new LockMessage(*this) ;
}

string LockMessage::toString()
{
    stringstream ss ;
    ss << MessageTuple::toString() << "(k=" << _k << ")" ;
    return ss.str() ;
}

string LockSnapshot::toString()
{
    stringstream ss;
    ss << _stateId << "(" ;

    if( _ss_t2 == -1 ) {
        if( _ss_ts == -1 )
            ss << "n/a,n/a" ;
        else
            ss << "ts,n/a" ;
    }
    else {
        if( _ss_t2 > _ss_ts )
            ss << "ts<t2" ;
        else if( _ss_t2 == _ss_ts )
            ss << "ts=t2"; // this should not happen
        else
            ss << "ts>t2" ;
    }
    
    
    ss << "," << _ss_old << "," << _ss_new << ")" ;
    return ss.str() ;
}