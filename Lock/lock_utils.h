//
//  lock_utils.h
//  Prob_verify
//
//  Created by Shou-pon Lin on 8/27/12.
//  Copyright (c) 2012 Shou-pon Lin. All rights reserved.
//

#ifndef LOCK_UTILS_H
#define LOCK_UTILS_H

#include "../statemachine.h"

namespace Lock_Utils {
    
    string getSingleParaName(int i, string tp);
    string getLockName(int i);
    string getChannelName(int i, int j);
    string getCompetitorName(int i);
    string getMsgType(const MessageTuple* tuple) ;
}
#endif
