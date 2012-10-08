//
//  lock_utils.cpp
//  Prob_verify
//
//  Created by Shou-pon Lin on 8/29/12.
//  Copyright (c) 2012 Shou-pon Lin. All rights reserved.
//

#include "lock_utils.h"
#include "lock.h"
#include "competitor.h"
#include "channel.h"
#include "controller.h"

string Lock_Utils::getSingleParaName(int i, string tp)
{
    stringstream ss;
    ss << tp << "(" << i << ")" ;
    return ss.str();
}

string Lock_Utils::getLockName(int i)
{
    return Lock_Utils::getSingleParaName(i , "lock");
}

string Lock_Utils::getChannelName(int i, int j)
{
    //stringstream ss;
    //ss << "channel(" << i << "," << j << ")" ;
    return string("channel");
}

string Lock_Utils::getCompetitorName(int i)
{
    return Lock_Utils::getSingleParaName(i, "competitor");
}

string Lock_Utils::getMsgType(const MessageTuple *tuple)
{
    if( typeid(*tuple) == typeid(CompetitorMessage))
        return string("CompetitorMessage") ;
    else if( typeid(*tuple) == typeid(ControllerMessage))
        return string("ControllerMessage") ;
    else if( typeid(*tuple) == typeid(LockMessage))
        return string("LockMessage");
    else
        return string("Others") ;
}

