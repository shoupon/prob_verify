//
//  channel.h
//  Prob_verify
//
//  Created by Shou-pon Lin on 8/29/12.
//  Copyright (c) 2012 Shou-pon Lin. All rights reserved.
//

#ifndef CHANNEL_H
#define CHANNEL_H

#include "../statemachine.h"
#include "lock.h"
#include "competitor.h"


class ChannelSnapshot;

class Channel: public StateMachine
{
public:
    Channel( int from, int to, Lookup* msg, Lookup* mac ) ;
    ~Channel() { if(_mem) delete _mem; }
    
    int transit(MessageTuple* inMsg, vector<MessageTuple*>& outMsgs,
                        bool& high_prob, int startIdx);
    int nullInputTrans(vector<MessageTuple*>& outMsgs,
                               bool& high_prob, int startIdx ) ;
    void restore(const StateSnapshot* snapshot);
    StateSnapshot* curState() ;
    // Reset the machine to initial state
    void reset() ;
    
protected:
    const int _from;
    const int _to;
    string _name;
    int _machineId;
    
    MessageTuple* _mem;
    
    MessageTuple* createDelivery() ;
};

// Used for restore the state of a state machine back to a certain point. Should contain
// the state in which the machine was, the internal variables at that point
class ChannelSnapshot: public StateSnapshot
{
    friend class Channel;
public:
    ChannelSnapshot() { _ss_mem = 0 ;}
    ChannelSnapshot( const ChannelSnapshot& item );
    ChannelSnapshot( const MessageTuple* msg ) { _ss_mem = msg->clone(); }
    ~ChannelSnapshot() { delete _ss_mem ;} 
    int curStateId() const { if( _ss_mem ) return _ss_mem->destMsgId(); else return 0; }
    // Returns the name of current state as specified in the input file
    string toString() { if(_ss_mem)return _ss_mem->toString(); else return string("empty");}
    // Cast the Snapshot into a integer. Used in HashTable
    int toInt() ;
    ChannelSnapshot* clone() const { return new ChannelSnapshot(*this) ; }
    
private:
    MessageTuple* _ss_mem;
};

#endif

