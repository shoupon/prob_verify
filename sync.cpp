//
//  sync.cpp
//  Prob_verify
//
//  Created by Shou-pon Lin on 3/7/13.
//  Copyright (c) 2013 Shou-pon Lin. All rights reserved.
//

#include <cassert>
#include <sstream>
using namespace std ;

#include "sync.h"


Sync::Sync( int numDeadline, Lookup* msg, Lookup* mac)
: StateMachine(msg,mac), _numDl(numDeadline)
{
    setId(machineToInt("sync"));
    reset() ;
}

int Sync::transit(MessageTuple* inMsg, vector<MessageTuple*>& outMsgs,
                  bool& high_prob, int startIdx)
{
    assert( typeid(*inMsg) == typeid(SyncMessage)) ;
    outMsgs.clear();
    high_prob = true ;
    
    int d = inMsg->getParam(1);
    assert( d <= _numDl ) ;
    
    bool set = inMsg->getParam(0) ;
    if( set ) {  // "SET"
        if( startIdx == 0 ) {
            // Block setting deadline 0 if the last sequence hasn't finished
            if (d == 0 && _nextDl != -1)
                return -1;
            // Change state
            _actives[d] = 1 ;   // setup deadline d
            _nextDl = getNextActive() ;
            return 3 ;
        }
        else {
            return -1;
        }
    }
    else {      // "REVOKE"
        if( startIdx == 0 ) {
            _actives[d] = 0 ;  // cancel deadline d
            _nextDl = getNextActive();
            return 3;
        }
        return -1;
    }
}

int Sync::nullInputTrans(vector<MessageTuple*>& outMsgs, bool& high_prob, int startIdx)
{
    high_prob = true ;
    outMsgs.clear() ;
    
    if( startIdx == 0 ) {
        if( _nextDl == -1 )   // There is no deadline that is currently active
            return -1;
        else {
            assert( _actives[_nextDl] == 1);
            // Generate deadline message
            for( int ii = 0 ; ii < _allMacs.size() ; ++ii ) {
                if (_allMacs[ii]._normal)
                    outMsgs.push_back(generateMsg(_allMacs[ii]._mac, DEADLINE, false, _nextDl)); 
            }
            _actives[_nextDl] = 0 ;
            getNextActive() ;
            _time++ ;
            
            return 1 ;
        }
        return -1 ;
    }
    else if (startIdx <= _failureGroups.size()) {
        vector<FailureGroup>::iterator it = _failureGroups.begin();
        int countDown = startIdx - 1;
        for (size_t gidx = 0; gidx < _failureGroups.size(); gidx++) {
            if (_failureGroups[gidx]._normal) {
                if (countDown == 0) {
                    failureEvent(gidx, outMsgs);
                    high_prob = false;
                    return startIdx + 1;
                }
                else {
                    countDown--;
                    continue;
                }
            }
        }
        return -1;
    }
    else
        return -1;
}


void Sync::restore(const StateSnapshot* snapshot)
{
    assert(typeid(*snapshot) == typeid(SyncSnapshot)) ;
    const SyncSnapshot* sss = dynamic_cast<const SyncSnapshot*>(snapshot) ;
    assert(sss->_ss_group_status.size() == _failureGroups.size());
    assert(sss->_ss_machine_status.size() == _allMacs.size());
    _actives = sss->_ss_act ;
    _nextDl = sss->_ss_next ;
    _time = sss->_ss_time;
    for (size_t ii = 0; ii < _failureGroups.size(); ++ii)
        _failureGroups[ii]._normal = sss->_ss_group_status[ii];
    for (size_t ii = 0; ii < _allMacs.size(); ++ii)
        _allMacs[ii]._normal = sss->_ss_machine_status[ii];
}

StateSnapshot* Sync::curState()
{
    return new SyncSnapshot( _actives, _nextDl, _failureGroups, _allMacs, _time ) ;
}

void Sync::reset()
{
    _actives.resize(0);
    _actives.resize(_numDl,0) ;
    _time = 0 ;
    _nextDl = -1;
    for (size_t ii = 0; ii < _allMacs.size(); ++ii)
        _allMacs[ii]._normal = true;
    for (size_t ii = 0; ii < _failureGroups.size(); ++ii)
        _failureGroups[ii]._normal = true;
}

// Associate the Sync (machine) with the pointer of the master (machine)
void Sync::setMaster(const StateMachine* master)
{
    _masterPtr = master ;
    MachineHandle handle;
    handle._mac = master;
    handle._normal = true;
    _allMacs.insert(_allMacs.begin(), handle);
}
void Sync::addMachine(const StateMachine* mac)
{
    MachineHandle handle;
    handle._mac = mac;
    handle._normal = true;
    _allMacs.push_back(handle) ;
}

void Sync::addFailureGroup(const vector<const StateMachine*>& machines)
{
    FailureGroup group;
    group._machines = machines;
    group._normal = true;
    _failureGroups.push_back(group);
}

SyncMessage* Sync::setDeadline(MessageTuple *inMsg, int macid, int did)
{
    if( inMsg == 0 )
        return new SyncMessage(0, machineToInt("sync"),
                               0, messageToInt("SET"),
                               macid, true, did);
    else
        return new SyncMessage(inMsg->srcID(), machineToInt("sync"),
                               inMsg->srcMsgId(), messageToInt("SET"),
                               macid, true, did);
}

// Revoke deadline[did] that corresponds to the deadline set by a machine
SyncMessage* Sync::revokeDeadline(MessageTuple *inMsg, int macid, int did)
{
    if( inMsg == 0 )
        return new SyncMessage(0, machineToInt("sync"),
                               0, messageToInt("REVOKE"),
                               macid, false, did);
    else
        return new SyncMessage(inMsg->srcID(), machineToInt("sync"),
                               inMsg->srcMsgId(), messageToInt("REVOKE"),
                               macid, false, did);
}

int Sync::getNextActive()
{
    for( int ai = 0 ; ai < (int)_actives.size() ; ++ai ) {
        if( _actives[ai] == 1 ) {
            _nextDl = ai;
            return ai;
        }
    }
    _nextDl = -1;
    return -1;
}

void Sync::failureEvent(size_t groupIdx, vector<MessageTuple*> &outMsgs)
{
    assert(_failureGroups[groupIdx]._normal);
    for (size_t midx = 0; midx < _failureGroups[groupIdx]._machines.size(); midx++) {
        const StateMachine* dest = _failureGroups[groupIdx]._machines[midx];
        outMsgs.push_back(generateMsg(dest, CLOCKFAIL,
                                      false, -1));
        bool found = false;
        for (size_t hidx = 0; hidx < _allMacs.size(); hidx++) {
            if (_allMacs[hidx]._mac == dest) {
                _allMacs[hidx]._normal = false;
                found = true;
                break;
            }
        }
        assert(found);
    }
    _failureGroups[groupIdx]._normal = false;
}

MessageTuple* Sync::generateMsg(const StateMachine *machine,
                                const string &msg, 
                                bool isSet, int did)
{
    return new SyncMessage(0, machine->macId(),
                           0, messageToInt(msg),
                           macId(), isSet, did) ;
}

     
/*
 * SyncMessage
 */

int SyncMessage::getParam(size_t arg)
{
    assert( arg == 0 || arg == 1);
    if( arg == 0 ) {
        if( _isSet )
            return 1 ;
        else
            return 0 ;
    }
    else if( arg == 1)
        return _deadlineId ;
    else return -1;
}

string SyncMessage::toString()
{
    stringstream ss;
    ss << MessageTuple::toString() ;
    ss << "(" << _isSet << "," << _deadlineId << ")" ;
    return ss.str();
}

/*
 * SyncSnapshot
 */

SyncSnapshot::SyncSnapshot( const SyncSnapshot& item )
{
    this->_ss_next = item._ss_next ;
    this->_ss_act = item._ss_act ;
    this->_ss_time = item._ss_time ;
    this->_ss_group_status = item._ss_group_status;
    this->_ss_machine_status = item._ss_machine_status;
}

SyncSnapshot::SyncSnapshot(const vector<int>& active, const int next, const vector<Sync::FailureGroup>& groups, const vector<Sync::MachineHandle>& handles, int time)
{
    this->_ss_act = active ;
    this->_ss_next = next ;
    this->_ss_time = time;
    for (size_t ii = 0; ii < groups.size(); ++ii)
        this->_ss_group_status.push_back(groups[ii]._normal);
    for (size_t ii = 0; ii < handles.size(); ++ii) 
        this->_ss_machine_status.push_back(handles[ii]._normal);
}

int SyncSnapshot::curStateId() const
{
    return 0 ;
}

string SyncSnapshot::toString()
{
    stringstream ss ;
    ss << "(" ;
    for( size_t ii = 0 ; ii < _ss_act.size() ; ++ii )
        ss << _ss_act[ii] ;
    ss << "," ;
    for (size_t ii = 0; ii < _ss_group_status.size(); ++ii) 
        ss << _ss_group_status[ii];
    ss << ",";
    for (size_t ii = 0; ii < _ss_machine_status.size(); ++ii)
        ss << _ss_machine_status[ii];
    ss << "," << _ss_next << ")" ;
    return ss.str() ;
}

int SyncSnapshot::toInt()
{
    int ret = 0 ;
    for( size_t ii = 0 ; ii < _ss_act.size() ; ++ii ) {
        ret = ret << 1;
        ret += _ss_act[ii] ;
    }
    ret = ret << 4 ;
    ret += _ss_next ;
    return ret ;
}
