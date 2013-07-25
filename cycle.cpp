//
//  cycle.cpp
//  verification
//
//  Created by Shou-pon Lin on 4/29/13.
//  Copyright (c) 2013 Shou-pon Lin. All rights reserved.
//

#include "cycle.h"

Cycle::Cycle( int numDeadline, Lookup* msg, Lookup* mac)
: Sync(numDeadline, msg, mac)
{
    setId(machineToInt("sync"));
    reset() ;
    _actives.resize(numDeadline, true);
    _nextDl = 0 ;
}

int Cycle::transit(MessageTuple* inMsg, vector<MessageTuple*>& outMsgs,
                  bool& high_prob, int startIdx)
{
    assert( typeid(*inMsg) == typeid(SyncMessage)) ;
    outMsgs.clear();
    high_prob = true ;
    return -1;
}

int Cycle::nullInputTrans(vector<MessageTuple*>& outMsgs, bool& high_prob, int startIdx)
{
    high_prob = true ;
    outMsgs.clear() ;
    
    if( startIdx == 0 ) {
        for( int ii = 0 ; ii < _allMacs.size() ; ++ii ) {
            MessageTuple* fire = new SyncMessage(0, _allMacs[ii]->macId(),
                                                 0, messageToInt("DEADLINE"),
                                                 macId(), false, _nextDl) ;
            outMsgs.push_back(fire);
        }
        
        _nextDl++;
        if( _nextDl == _numDl )
            _nextDl = 0 ;
        
        return 3;
    }
    else
        return -1 ;
}

void Cycle::reset()
{
    _time = 0 ;
    _nextDl = 0;
}