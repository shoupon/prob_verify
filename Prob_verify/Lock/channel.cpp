//
//  channel.cpp
//  Prob_verify
//
//  Created by Shou-pon Lin on 8/29/12.
//  Copyright (c) 2012 Shou-pon Lin. All rights reserved.
//

#include "channel.h"

Channel::Channel(int num, Lookup* msg, Lookup* mac)
: StateMachine(msg,mac), _range(num), _mem(0)
{
    // The name of the lock is "lock(i)", where i is the id of the machine
    _name = Lock_Utils::getChannelName(num, num) ;
    _machineId = machineToInt(_name);
}

int Channel::transit(MessageTuple* inMsg, vector<MessageTuple*>& outMsgs,
                     bool& high_prob, int startIdx)
{
    outMsgs.clear();
    
    int incomingMsg = inMsg->destMsgId() ;
    if( incomingMsg == 8 ) {
        incomingMsg = 3;
    }
        

    if( _mem == 0 ) {
        // Channel is empty
        if( startIdx == 0 ) {
            // Transimission succeeds
            high_prob = true ;
            // Change state
            _mem = inMsg->clone() ;
            return 1;
        }
        else if( startIdx == 1 ) {
            // Message loss
            high_prob = false;
            // channel remain empty
            _mem = 0 ;
            return 2;
        }
        else if(startIdx >=2 ) {
            return -1;
        }
        else {
            return -1;
        }
    }
    else {
        // Channel is not empty
        return -1;
    }
}

int Channel::nullInputTrans(vector<MessageTuple*>& outMsgs, bool& high_prob, int startIdx)
{
    outMsgs.clear() ;
    high_prob = true ;

    if( _mem != 0 ) {
        if( startIdx == 0 ) {
            MessageTuple* msg = createDelivery() ;
            outMsgs.push_back(msg);

            // Change state
            delete _mem ;
            _mem = 0;
            return 3;
        }
        else if( startIdx > 0 ) {
            return -1;
        }
        else {
            return -1;
        }
    }
    else {
        return -1;
    }
}

void Channel::restore(const StateSnapshot* snapshot)
{
    assert( typeid(*snapshot) == typeid(ChannelSnapshot));
    const ChannelSnapshot* css = dynamic_cast<const ChannelSnapshot*>(snapshot) ;
    
    if( css->_ss_mem == 0 )
        _mem = 0;
    else
        _mem = css->_ss_mem->clone() ;
}

StateSnapshot* Channel::curState()
{
    if( _mem == 0 )
        return new ChannelSnapshot();
    else
        return new ChannelSnapshot(_mem);
}

void Channel::reset()
{
    if( _mem )
        delete _mem;
}

MessageTuple* Channel::createDelivery()
{
    int outMsgId = _mem->destMsgId();
    assert(_mem->destId() == _machineId);
    
    // The message stored in _mem should be either of type CompetitorMessage or of type
    // LockMessage
    int toward = _mem->getParam(1);
    
    MessageTuple* ret ;
    if( typeid(*_mem) == typeid(CompetitorMessage) ) {
        // This message destined for a lock
        string lockName = Lock_Utils::getLockName(toward) ;
        int dstId = machineToInt(lockName);
        
        CompetitorMessage* compMsgPtr = dynamic_cast<CompetitorMessage*>(_mem) ;
        ret = new CompetitorMessage(0,dstId,0,outMsgId,_machineId, *compMsgPtr);
    }
    else if( typeid(*_mem) == typeid(LockMessage) ) {
        // This message destined for a competitor
        string compName = Lock_Utils::getCompetitorName(toward);
        int dstId = machineToInt(compName) ;
        
        LockMessage* lockMsgPtr = dynamic_cast<LockMessage*>(_mem);
        ret = new LockMessage(0,dstId,0,outMsgId,_machineId, *lockMsgPtr);
    }
    else {
        throw runtime_error("The message stored in channel belongs to unxpected type") ;
    }
    
    return ret;
}

ChannelSnapshot::ChannelSnapshot( const ChannelSnapshot& item )
{
    if( item._ss_mem )
        _ss_mem = item._ss_mem->clone() ;
    else
        _ss_mem = 0 ;
}

int ChannelSnapshot::toInt() 
{ 
    if( _ss_mem ) 
        return _ss_mem->srcMsgId() + _ss_mem->destMsgId(); 
    else
        return 0 ;
}