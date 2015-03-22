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

#define SYNC_NAME "sync"

int Sync::recurring_ = 0;

Sync::Sync(int numDeadline)
    : _numDl(numDeadline), detectable_failure_prob_(3),
      fatal_failure_prob_(6) {
  setId(machineToInt(SYNC_NAME));
  machine_name_ = SYNC_NAME;
  reset() ;
}

Sync::Sync(int num_deadlines,
           int detectable_failure_prob, int fatal_failure_prob)
    : _numDl(num_deadlines),
      detectable_failure_prob_(detectable_failure_prob),
      fatal_failure_prob_(fatal_failure_prob) {
  setId(machineToInt(SYNC_NAME));
  machine_name_ = SYNC_NAME;
  reset();
}

int Sync::transit(MessageTuple* in_msg, vector<MessageTuple*>& out_msgs,
                  int& prob_level, int start_idx) {
    assert( typeid(*in_msg) == typeid(SyncMessage)) ;
    out_msgs.clear();
    prob_level = 0;
    
    int d = in_msg->getParam(1);
    assert( d <= _numDl ) ;
    
    bool set = in_msg->getParam(0) ;
    if (set) {  // "SET"
        if (!start_idx) {
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
        if (!start_idx) {
            _actives[d] = 0 ;  // cancel deadline d
            _nextDl = getNextActive();
            return 3;
        }
        return -1;
    }
}

int Sync::nullInputTrans(vector<MessageTuple*>& out_msgs,
                         int& prob_level, int start_idx) {
  prob_level = 0;
  out_msgs.clear() ;
  
  if (start_idx <= _failureGroups.size() << 1) {
    int countDown = start_idx >> 1;
    // Clock failure events
    for (size_t gidx = 0; gidx < _failureGroups.size(); gidx++) {
      if (_failureGroups[gidx]._normal) {
        if (countDown == 0 && (start_idx & 1) == 0) {
          failureEvent(gidx, out_msgs);
          prob_level = detectable_failure_prob_;
          return start_idx + 1;
        } else if (countDown == 0 && (start_idx & 1)) {
          failureEventFatal(gidx);
          prob_level = fatal_failure_prob_;
          return start_idx + 1;
        }
        else {
          countDown--;
          continue;
        }
      }
    }
    // Deadline expiry events
    if (_nextDl != -1) {
      assert( _actives[_nextDl] == 1);
      // Generate deadline message
      for (int ii = 0 ; ii < _allMacs.size() ; ++ii) {
        if (_allMacs[ii]._normal)
          out_msgs.push_back(generateMsg(_allMacs[ii]._mac, DEADLINE,
                                         false, _nextDl)); 
      }
      if (!recurring_) {
        _actives[_nextDl] = 0 ;
        _time++ ;
      }
      getNextActive() ;
      return (_failureGroups.size() + 1) << 1;
    }
    else 
        return -1;
  } else {
    return -1;
  }
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

void Sync::reset() {
  _actives.resize(0);
  _time = 0;
  if (recurring_) {
    _actives.resize(_numDl, 1);
    _nextDl = 0;
  } else {
    _actives.resize(_numDl, 0);
    _nextDl = -1;
  }
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

bool Sync::isAvailable(int deadline_id) {
  return !_actives[deadline_id];
}

int Sync::getNextActive() {
  for( int ai = 0 ; ai < (int)_actives.size() ; ++ai ) {
    if( _actives[ai] == 1 ) {
      _nextDl = ai;
      return ai;
    }
  }
  if (recurring_)
    return _nextDl = 0;
  else
    return _nextDl = -1;
}

void Sync::failureEvent(size_t group_idx, vector<MessageTuple*> &outMsgs) {
  assert(_failureGroups[group_idx]._normal);
  for (const auto dest : _failureGroups[group_idx]._machines)
    outMsgs.push_back(generateMsg(dest, CLOCKFAIL, false, -1));
  failureEventFatal(group_idx);
}

void Sync::failureEventFatal(size_t group_idx) {
    assert(_failureGroups[group_idx]._normal);
    for (const auto dest : _failureGroups[group_idx]._machines) {
        bool found = false;
        for (auto& machine : _allMacs) {
          if (machine._mac == dest) {
            machine._normal = false;
            found = true;
            break;
          }
        }
        assert(found);
    }
    _failureGroups[group_idx]._normal = false;
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

string SyncMessage::toString() const {
  stringstream ss;
  ss << MessageTuple::toString();
  ss << "(" << _isSet << "," << _deadlineId << ")";
  return ss.str();
}

string SyncMessage::toReadable() const {
  stringstream ss;
  ss << MessageTuple::toReadable();
  ss << "(" << _isSet << "," << _deadlineId << ")";
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

size_t SyncSnapshot::getBytes() const {
  size_t sz = sizeof(int) * 2 + sizeof(_ss_act) + sizeof(_ss_group_status)
      + sizeof(_ss_machine_status);
  sz += sizeof(int) * _ss_act.size();
  sz += sizeof(bool) * _ss_group_status.size();
  sz += sizeof(bool) * _ss_machine_status.size();
  return sz;
}

string SyncSnapshot::toString() const {
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
