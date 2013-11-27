//
//  service.cpp
//  verification-lock
//
//  Created by Shou-pon Lin on 11/19/13.
//  Copyright (c) 2013 Shou-pon Lin. All rights reserved.
//

#include "service.h"

set<string> Service::_traversed;
Service::~Service()
{
    MsgSet::iterator it;
    for( it = _interface.begin(); it != _interface.end() ; ++it)
        delete *it;
}

void Service::putMsg(MessageTuple *msg)
{
    vector<MessageTuple *> outMsgs;
    bool high_prob;
    if (this->isMonitored(msg))     // Dynamically calling the function isMonitored()
        transit(msg, outMsgs, high_prob);
}

bool Service::isMonitored(MessageTuple *msg)
{
    return _interface.find(msg) != _interface.end();
}

static void print(string label)
{
    cout << label << endl;
}
void Service::printTraversed()
{
    for_each(_traversed.begin(), _traversed.end(), &print);
}