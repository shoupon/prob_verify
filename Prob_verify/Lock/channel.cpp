//
//  channel.cpp
//  Prob_verify
//
//  Created by Shou-pon Lin on 8/29/12.
//  Copyright (c) 2012 Shou-pon Lin. All rights reserved.
//

#include "channel.h"

Channel::Channel(int num, Lookup* msg, Lookup* mac)
: StateMachine(msg,mac), _range(num)
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
        

    if( _mem.size() < 10000 ) {
        // Channel is empty
        if( startIdx == 0 ) {
            // Transimission succeeds
            high_prob = true ;
            // Change state
            _mem.push_back( inMsg->clone() );
            return 1;
        }
        else if( startIdx == 1 ) {
            // Message loss
            high_prob = false;
            // channel content remains the same
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

    if( !_mem.empty() ) {
        if( startIdx == 0 ) {
            // Create message
            MessageTuple* msg = createDelivery() ;
            outMsgs.push_back(msg);

            // Change state
            delete _mem.front();
            _mem.erase(_mem.begin()) ;
            
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
    
    if( css->_ss_mem.empty() )
        clearMem(_mem) ;
    else
        copyMem(css->_ss_mem, _mem);
}

StateSnapshot* Channel::curState()
{
    if( _mem.empty() )
        return new ChannelSnapshot();
    else
        return new ChannelSnapshot(_mem);
}

void Channel::reset()
{
    clearMem(_mem);
}

void Channel::copyMem(const vector<MessageTuple*>& fifo, vector<MessageTuple*>& dest)
{
    clearMem(dest);
    dest.resize(fifo.size()) ;
    
    for( size_t i = 0 ; i < fifo.size() ; ++i ) {
        dest[i] = fifo[i]->clone() ;
    }
}
void Channel::clearMem(vector<MessageTuple*>& fifo)
{
    for( size_t i = 0 ; i < fifo.size() ; ++i ) {
        delete fifo[i];
    }
    fifo.clear() ;
}

MessageTuple* Channel::createDelivery()
{
    if( _mem.empty() )
        return 0;
    
    MessageTuple* msg = _mem.front() ;
    
    int outMsgId = msg->destMsgId();
    
    // The message stored in _mem should be either of type CompetitorMessage or of type
    // LockMessage
    int toward = msg->getParam(1);
    
    MessageTuple* ret ;
    if( typeid(*msg) == typeid(CompetitorMessage) ) {
        // This message destined for a lock
        string lockName = Lock_Utils::getLockName(toward) ;
        int dstId = machineToInt(lockName);
        
        CompetitorMessage* compMsgPtr = dynamic_cast<CompetitorMessage*>(msg) ;
        ret = new CompetitorMessage(0,dstId,0,outMsgId,_machineId, *compMsgPtr);
    }
    else if( typeid(*msg) == typeid(LockMessage) ) {
        // This message destined for a competitor
        string compName = Lock_Utils::getCompetitorName(toward);
        int dstId = machineToInt(compName) ;
        
        LockMessage* lockMsgPtr = dynamic_cast<LockMessage*>(msg);
        ret = new LockMessage(0,dstId,0,outMsgId,_machineId, *lockMsgPtr);
    }
    else {
        throw runtime_error("The message stored in channel belongs to unxpected type") ;
    }
    
    return ret;
}

ChannelSnapshot::ChannelSnapshot( const ChannelSnapshot& item )
{
    if( !item._ss_mem.empty() )
        Channel::copyMem(item._ss_mem, _ss_mem);
}

ChannelSnapshot::ChannelSnapshot( const vector<MessageTuple*>& fifo)
{
    if( !fifo.empty())
        Channel::copyMem(fifo, _ss_mem);
}

int ChannelSnapshot::curStateId() const
{
    if( !_ss_mem.empty() )
        return _ss_mem.front()->destMsgId();
    else
        return 0;
}

string ChannelSnapshot::toString()
{
    stringstream ss ;
    ss << "(" ;

    for( size_t i  = 0 ; i < _ss_mem.size() ; ++i ) {
        ss << _ss_mem[i]->toString() ;
        if( i != _ss_mem.size()-1 )
            ss << "," ;
    }
    ss << ")" ;
        
    return ss.str() ;
}

int ChannelSnapshot::toInt()
{ 
    if( !_ss_mem.empty() ) {
        int ret = 0 ;
        for( size_t i = 0 ; i < _ss_mem.size() ; ++i ) {
            ret += _ss_mem[i]->destMsgId() ;
            ret = ret << 4 ;
            ret += _ss_mem[i]->destId() ;
            ret = ret << 4 ;
        }
        return ret ;
    }
    else
        return 0 ;
}

