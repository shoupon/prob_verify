//
//  heartbeat.cpp
//  Prob_verify
//
//  Created by Shou-pon Lin on 10/16/12.
//  Copyright (c) 2012 Shou-pon Lin. All rights reserved.
//

#include <sstream>
using namespace std;

#include "heartbeat.h"
#include "lock_utils.h"

int Heartbeater::transit(MessageTuple* inMsg, vector<MessageTuple*>& outMsgs,
                  bool& high_prob, int startIdx )
{
    outMsgs.clear();
    
    if( startIdx != 0 )
        return -1 ;
    
    high_prob = true ;
    
    string msg = IntToMessage(inMsg->destMsgId() ) ;
    int k = inMsg->getParam(0) ;
    if( msg == "start" ) {
        if( _beating[k] == 0 ) {
            // Change State
            _beating[k] = 1;

            return 3;
        }
    }
    else if( msg == "stop" ) {
        if( _beating[k] == 1 ) {
            // Change State
            _beating[k] = 0 ;
            
            return 3;
        }
    }
                
    return -1;
}

int Heartbeater::nullInputTrans(vector<MessageTuple *> &outMsgs,
                                bool &high_prob, int startIdx)
{
    if( startIdx < 0 || startIdx >= _beating.size() )
        return -1;
    
    outMsgs.clear();
    
    int k = 0 ;
    int ix = startIdx + 1 ;
    while(true){
        if( k >= _beating.size() ) {
            return -1;
        }
        
        if( _beating[k] == 1 ) {
            ix--;
            if( ix == 0 )
                break;
        }
        
        k++;
    }
    
    MessageTuple* res = new HBMessage(0, machineToInt( Lock_Utils::getLockName(k) ),
                                      0, messageToInt("dead"),
                                      _machineId);
    outMsgs.push_back(res);
    _beating[k] = 0 ;
    high_prob = false;
    
    return startIdx + 1;
}

void Heartbeater::restore(const StateSnapshot *snapshot)
{
    assert( typeid(*snapshot) == typeid(HBSnapshot)) ;
    
    const HBSnapshot* hbss = dynamic_cast<const HBSnapshot*>(snapshot) ;
    _beating = hbss->_ss_beating ;
}

int HBSnapshot::curStateId() const
{
    int ret = 0 ;
    for( size_t i = 0 ; i < _ss_beating.size() ; ++i ) {
        ret += _ss_beating[i] ;
        ret = ret << 2;
    }
    
    return ret;
}

string HBSnapshot::toString()
{
    stringstream ss;
    ss << "(" ;
    for( size_t i = 0 ; i < _ss_beating.size() -1 ; ++i ) {
        ss << _ss_beating[i] << "," ;
    }
    ss << _ss_beating.back() << ")" ;
    return ss.str() ;
}

int HBSnapshot::toInt()
{
    return curStateId() ;
}
