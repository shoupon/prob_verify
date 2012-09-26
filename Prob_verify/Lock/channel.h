//
//  channel.h
//  Prob_verify
//
//  Created by Shou-pon Lin on 8/29/12.
//  Copyright (c) 2012 Shou-pon Lin. All rights reserved.
//

#ifndef CHANNEL_H
#define CHANNEL_H

#include <vector>
using namespace std;

#include "../statemachine.h"
#include "lock.h"
#include "competitor.h"


class ChannelSnapshot;


class Channel: public StateMachine
{
public:
    Channel( int num, Lookup* msg, Lookup* mac ) ;
    ~Channel() { clearMem(_mem); }
    
    int transit(MessageTuple* inMsg, vector<MessageTuple*>& outMsgs,
                        bool& high_prob, int startIdx);
    int nullInputTrans(vector<MessageTuple*>& outMsgs,
                               bool& high_prob, int startIdx ) ;
    void restore(const StateSnapshot* snapshot);
    StateSnapshot* curState() ;
    // Reset the machine to initial state
    void reset() ;
    
    static void copyMem(const vector<MessageTuple*>& fifo, vector<MessageTuple*>& dest) ;
    static void clearMem(vector<MessageTuple*>& fifo);
    
protected:
    const int _range;
    string _name;
    int _machineId;
    
    vector<MessageTuple*> _mem;
    
    MessageTuple* createDelivery(int idx) ;
};

// Used for restore the state of a state machine back to a certain point. Should contain
// the state in which the machine was, the internal variables at that point
class ChannelSnapshot: public StateSnapshot
{
    friend class Channel;
public:
    ChannelSnapshot() {}
    ChannelSnapshot( const ChannelSnapshot& item );
    ChannelSnapshot( const vector<MessageTuple*>& fifo);
    ~ChannelSnapshot() { Channel::clearMem(_ss_mem) ;}
    int curStateId() const ; 
    // Returns the name of current state as specified in the input file
    string toString() ;
    // Cast the Snapshot into a integer. Used in HashTable
    int toInt() ;
    ChannelSnapshot* clone() const { return new ChannelSnapshot(*this) ; }
    
private:
    vector<MessageTuple*> _ss_mem;
};

#endif

