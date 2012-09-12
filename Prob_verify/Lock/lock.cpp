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
                int t = inMsg->getParam(1) ;
                _ts = t ;
                _old = i;
                // Response
                if( _old != _id ) {
                    MessageTuple* response = createResponse("LOCKED", "channel", inMsg, _old);
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
                    MessageTuple* response = createResponse("free", ownComp, inMsg, _id);
                    outMsgs.push_back(response);
                    
                    return 3;
                }
            }
            break;
        
        case 1:
            if( msg == "RELEASE" ) {
                if( inMsg->getParam(0) == _old ) {
                    // Response
                    MessageTuple* ctrlRes = new LockMessage(0, machineToInt("controller"),
                                                            0, messageToInt("complete"),
                                                            _machineId, _id, -1);
                    outMsgs.push_back(ctrlRes);
                    // Change state
                    _current = 0;
                    reset();
                    
                    return 3;
                }
            }
            else if( msg == "status" ) {
                assert( typeid(*inMsg) == typeid(CompetitorMessage)) ;
                // Response
                if(  inMsg->getParam(0) == _id ) {
                    string ownComp = Lock_Utils::getCompetitorName(_id) ;
                    MessageTuple* response = createResponse("secured", ownComp, inMsg, _id);
                    outMsgs.push_back(response);
                    
                    return 3;
                }
            }
            else if( msg == "REQUEST" ) {
                int j = inMsg->getParam(0) ;
                if( j != _old ) {
                    // Assignments
                    _t2 = inMsg->getParam(1);
                    _new = j ;
                    
                    if( _t2 < _ts ) {
                        // True
                        // Response
                        MessageTuple* react = createResponse("INQUIRE", "channel",
                                                             inMsg, _old );
                        outMsgs.push_back(react);
                        
                        // Change state
                        _current = 2;
                    }
                    else {
                        // False
                        // Response
                        MessageTuple* response = createResponse("FAILED", "channel",
                                                                inMsg, _new );
                        outMsgs.push_back(response);
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
                // Change state
                _current = 0;
                reset();
                
                return 3;
            }
            
        case 2:
            if( msg == "ENGAGED" ) {
                if( inMsg->getParam(0) == _old) {
                    // Respond
                    MessageTuple* react = createResponse("FAILED", "channel", inMsg, _new) ;
                    outMsgs.push_back(react);
                    // Assign variables
                    _new = _t2 = -1;
                    // Change state
                    _current = 1;
                    
                    return 3;
                }
            }
            else if( msg == "RELEASE" ) {
                if( inMsg->getParam(0) == _old ) {
                    // Respond
                    MessageTuple* react = createResponse("LOCKED", "channel", inMsg, _new);
                    outMsgs.push_back(react);
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
                    MessageTuple* response = createResponse("secured", ownComp, inMsg, _id);
                    outMsgs.push_back(response);
                    
                    return 3;
                }
            }
            else if( msg == "timeout" ) {
                assert( inMsg->subjectId() == machineToInt("time") ) ;
                // Change state
                _current = 0;
                reset();
                
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
}

MessageTuple* Lock::createResponse(string msg, string dst, MessageTuple* inMsg, int toComp)
{
    int outMsgId = messageToInt(msg);
    int dstId = machineToInt(dst);
    
    assert(inMsg->destId() == _machineId);
    assert(toComp < _range) ;
    
    MessageTuple* ret = new LockMessage(inMsg->subjectId(), dstId,
                                        inMsg->destMsgId(), outMsgId,
                                        _machineId, _id, toComp);
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