//
//  lock_utils.cpp
//  Prob_verify
//
//  Created by Shou-pon Lin on 8/29/12.
//  Copyright (c) 2012 Shou-pon Lin. All rights reserved.
//

#include "lock_utils.h"

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