//
//  competitor.cpp
//  Prob_verify
//
//  Created by Shou-pon Lin on 8/28/12.
//  Copyright (c) 2012 Shou-pon Lin. All rights reserved.
//

#include "competitor.h"
#include "lock_utils.h"

#include <iostream>
using namespace std;

Competitor::Competitor(int k, int delta, int num, Lookup* msg, Lookup* mac)
:_id(k), _delta(delta), _range(num), StateMachine(msg,mac)
{
    // The name of the lock is "lock(i)", where i is the id of the machine
    _name = Lock_Utils::getCompetitorName(_id);
    _machineId = machineToInt(_name);
    reset();
}

int Competitor::transit(MessageTuple *inMsg, vector<MessageTuple *> &outMsgs,
                        bool &high_prob, int startIdx)
{
    if( startIdx != 0 )
        return -1 ;
    
    outMsgs.clear();
    high_prob = true ;
    
    string msg = IntToMessage(inMsg->destMsgId() ) ;
    switch ( _current ) {
        case 0 :
            if( msg == "init" ) {
                //assert( typeid(*inMsg) == typeid(ServiceMessage) );
                assert( inMsg->numParams() == 3 ) ;
                // Assignments
                _t = inMsg->getParam(0);
                _front = inMsg->getParam(1);
                _back = inMsg->getParam(2);
                // Response
                MessageTuple* response = createResponse("status",
                                                        Lock_Utils::getLockName(_id),
                                                        inMsg, _id);
                outMsgs.push_back(response);
                // Change State
                _current = 1;
                
                return 3;
            }
            else if( msg == "LOCKED" || msg == "FAILED" || msg == "timeout" ) {
                // Do nothing
                return 3;
            }
            else if( msg == "INQUIRE" ) {
                // Respond
                int lock = inMsg->getParam(0) ;
                int relId = messageToInt("RELEASE");
                MessageTuple* kRel =
                    new CompetitorMessage(0, machineToInt("channel"),
                                          0, relId, _machineId, _id, lock,
                                          inMsg->getParam(2));
                outMsgs.push_back(kRel);
                
                return 3;
            }
            break;
            
        case 1:
            if( msg == "free" ) {
                if( inMsg->getParam(0) == _id ) {
                    // The free message should always come from own lock
                    // Response
                    MessageTuple* rOwn = createReq(Lock_Utils::getLockName(_id), inMsg, _id);
                    MessageTuple* rFront = createReq("channel", inMsg, _front);
                    MessageTuple* rBack = createReq("channel", inMsg, _back);
                    outMsgs.push_back(rOwn);
                    outMsgs.push_back(rFront);
                    outMsgs.push_back(rBack);
                    // Change state
                    _current = 2;
                    
                    return 3;
                }
            }
            else if( msg == "secured" ) {
                // Response
                if(  inMsg->getParam(0) == _id ) {
                    // Change state
                    _current = 0;
                    reset();
                    
                    return 3;
                }
            }
            break;

        case 2:
            if( msg == "LOCKED" ) {
                if( inMsg->getParam(2) == _t ){
                    if( inMsg->getParam(0) == _front) {
                        // Change state
                        _current = 3;
                        
                        return 3;
                    }
                    else if( inMsg->getParam(0) == _back ) {
                        // Change state
                        _current = 13;
                        
                        return 3;
                    }
                }
                else {
#ifdef VERBOSE_ACTIONS
                    cout << "Received LOCKED from earlier transaction (t="
                         << inMsg->getParam(2)
                         << "), current transaction t=" << _t << endl;
#endif
                    // Ignore the outdated LOCKED message
                    return 3;
                }
            }
            else if( toRelease(inMsg, outMsgs) ) {
                return 3;
            }
            else if( toTimeout(inMsg, outMsgs) ) {
                return 3;
            }
            
            break;
            
        case 3: // State s_3
            if( msg == "LOCKED" ) {
                if( inMsg->getParam(2) == _t ) {
                    if( inMsg->getParam(0) == _back) {
                        // Change state
                        _current = 4;
                        
                        return 3;
                    }
                }
                else {
                    // outdated LOCKED, ignore
                    return 3;
                }
            }
            else if( toRelease(inMsg, outMsgs) ) {
                return 3;
            }
            else if( toTimeout(inMsg, outMsgs) ) {
                return 3;
            }
            
            break;
            
        case 13: // State s_3'
            if( msg == "LOCKED" ) {
                if( inMsg->getParam(2) == _t ) {
                    if( inMsg->getParam(0) == _front) {
                        // Change state
                        _current = 4;
                        
                        return 3;
                    }
                }
                else {
                    // outdated LOCKED, ignore
                    return 3;
                }
            }
            else if( toRelease(inMsg, outMsgs) ) {
                return 3;
            }
            else if( toTimeout(inMsg, outMsgs) ) {
                return 3;
            }
            
            break;
            
        case 4:
            if( toTimeout(inMsg, outMsgs) ) {
                return 3;
            }
            else if( msg == "LOCKED" ) {
                // Simply ignore this message
                return 3;
            }
            else if( msg == "INQUIRE" ) {
                if( inMsg->getParam(2) == _t ) {
                    // response
                    MessageTuple* eng = createResponse("ENGAGED", "channel", inMsg,
                                                       inMsg->getParam(2) );
                    outMsgs.push_back(eng);
                    // variables and state remain the same
                    return 3;
                }
                else {
                    // different timestamps mean the lock thinks that it is in a different
                    // transaction. Which is WRONG
                    assert(false);
                    return 3 ;
                }
                
            }
            break;
                                
        default:
            return -1;
            break;
    }
    
    return -1;

    
}

int Competitor::nullInputTrans(vector<MessageTuple *> &outMsgs, bool &high_prob,
                               int startIdx)
{
    outMsgs.clear();
    int relId = messageToInt("RELEASE");
    
    if( startIdx == 0 ) {
        if( _current == 4 ) {
            MessageTuple* ctrlRes = new CompetitorMessage(0, machineToInt("controller"),
                                                          0, messageToInt("complete"),
                                                          _machineId, _id, -1, 0);
            MessageTuple* ownRel =
                new CompetitorMessage(0, machineToInt(Lock_Utils::getLockName(_id)),
                                      0, relId, _machineId, _id, _id, _t);
            MessageTuple* fRel =
                new CompetitorMessage(0, machineToInt("channel"),
                                      0, relId, _machineId, _id, _front, _t);
            
            MessageTuple* bRel =
                new CompetitorMessage(0, machineToInt("channel"),
                                      0, relId, _machineId, _id, _back, _t);
            
            outMsgs.push_back(ctrlRes);
            outMsgs.push_back(ownRel);
            outMsgs.push_back(fRel);
            outMsgs.push_back(bRel);
            // Change State
            _current = 0;
            reset();
            
            return 3;
            
        }
    }
    
    
    return -1;
}

void Competitor::restore(const StateSnapshot* snapshot)
{
    assert( typeid(*snapshot) == typeid(CompetitorSnapshot)) ;
    const CompetitorSnapshot* img = dynamic_cast<const CompetitorSnapshot*>(snapshot) ;
    _t = img->_ss_t ;
    _front = img->_ss_f ;
    _back = img->_ss_b ;
    _current = img->_stateId ;
}

StateSnapshot* Competitor::curState()
{
    StateSnapshot* ret = new CompetitorSnapshot(_t, _front, _back, _current) ;
    return ret; 
}

void Competitor::reset()
{
    _t = _front = _back = -1;
    _current = 0;
}

MessageTuple* Competitor::createResponse(string msg, string dst, MessageTuple* inMsg,
                                         int toLock)
{
    int outMsgId = messageToInt(msg);
    int dstId = machineToInt(dst);
    
    assert(inMsg->destId() == _machineId);
    
    MessageTuple* ret = new CompetitorMessage(inMsg->subjectId(), dstId,
                                              inMsg->destMsgId(), outMsgId,
                                              _machineId, _id, toLock, _t);
    return ret;
}

MessageTuple* Competitor::createReq(string dst, MessageTuple* inMsg, int toLock)
{
    // Create a typical message with source identifier
    MessageTuple* ret = createResponse("REQUEST", dst, inMsg, toLock);
    // Add timestamp onto the message
    //CompetitorMessage* compMsg = dynamic_cast<CompetitorMessage*>(ret) ;
    //compMsg->createReq(_t) ;
    return ret ;
}

bool Competitor::toRelease(MessageTuple *inMsg, vector<MessageTuple *> &outMsgs)
{
    string msg = IntToMessage(inMsg->destMsgId() ) ;
    assert( outMsgs.size() == 0 );
    
    int from = inMsg->getParam(0);
    if( msg == "INQUIRE" && (from==_front || from==_back) ) {
        if(inMsg->getParam(2)==_t) {
            // Response
            MessageTuple* rFront = createResponse("RELEASE", "channel", inMsg, _front);
            MessageTuple* rBack = createResponse("RELEASE", "channel", inMsg, _back);
            MessageTuple* rOwn = createResponse("RELEASE", Lock_Utils::getLockName(_id),
                                                inMsg, _id );
            outMsgs.push_back(rFront);
            outMsgs.push_back(rBack);
            outMsgs.push_back(rOwn);
            outMsgs.push_back(abortMsg());
            
        }
        else {
            // ignore the message with wrong timestamp
            return true ;
        }
    }
    else if( msg == "FAILED" ) {
        if(inMsg->getParam(2)==_t) {
            if( from == _front ) {
                MessageTuple* rBack = createResponse("RELEASE", "channel", inMsg, _back);
                MessageTuple* rOwn = createResponse("RELEASE", Lock_Utils::getLockName(_id),
                                                    inMsg, _id );
                outMsgs.push_back(rBack);
                outMsgs.push_back(rOwn);
                outMsgs.push_back(abortMsg());
            }
            else if( from == _back ) {
                MessageTuple* rFront = createResponse("RELEASE", "channel", inMsg, _front);
                MessageTuple* rOwn = createResponse("RELEASE", Lock_Utils::getLockName(_id),
                                                    inMsg, _id );
                outMsgs.push_back(rFront);
                outMsgs.push_back(rOwn);
                outMsgs.push_back(abortMsg());
            }
        }
        else {
            // ignore the message with wrong timestamp
            return true ;
        }
    }
    
    
    if( outMsgs.size() != 0 ) {
        _current = 0;
        reset();
        return true ;        
    }
    else
        return false;
}

bool Competitor::toTimeout(MessageTuple *inMsg, vector<MessageTuple *> &outMsgs)
{
    string msg = IntToMessage(inMsg->destMsgId() ) ;
    assert( outMsgs.size() == 0 );
    
    if( msg == "timeout" && inMsg->getParam(0) == _t ) {
        // Response
        _current = 0;
        reset();
        return true;
    }
    
    return false ;
}
CompetitorMessage* Competitor::abortMsg()
{
    return new CompetitorMessage(0, machineToInt("controller"),
                                 0, messageToInt("abort"),
                                 _machineId, _id, -1, _t);
}

/*
CompetitorMessage* CompetitorMessage::createReq(int timeGen)
{
    _nParams = 3;
    _t = timeGen ;
    return this;
}*/

string CompetitorMessage::toString()
{
    stringstream ss ;
    ss << MessageTuple::toString() << "(k=" << _k << ",l=" << _lock << ")" ;
    return ss.str() ;
    
}

CompetitorMessage* CompetitorMessage::clone() const
{
    CompetitorMessage* ret = new CompetitorMessage(*this);
    return ret;
}


string CompetitorSnapshot::toString()
{
    stringstream ss;
    ss << _stateId << "(" << _ss_f << "," << _ss_b << ")" ;
    return ss.str() ;
}