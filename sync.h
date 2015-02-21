//
//  sync.h
//  Prob_verify
//
//  Created by Shou-pon Lin on 3/7/13.
//  Copyright (c) 2013 Shou-pon Lin. All rights reserved.
//


#ifndef SYNC_H
#define SYNC_H

#include <typeinfo>
#include <vector>
using namespace std;

#include "statemachine.h"

#define DEADLINE "DEADLINE"
#define SET "SET"
#define CLOCKFAIL "CLOCKFAIL"

class SyncSnapshot;
class SyncMessage;

class Sync: public StateMachine {
  static int recurring_;
public:
    struct MachineHandle
    {
        bool _normal;
        const StateMachine* _mac;
        const StateMachine* operator->() const { return _mac; }
    };

    struct FailureGroup
    {
        bool _normal;
        vector<const StateMachine*> _machines;
    };

    Sync( int numDeadline, Lookup* msg, Lookup* mac ) ;
    ~Sync() { }
    
    int transit(MessageTuple* inMsg, vector<MessageTuple*>& outMsgs,
                bool& high_prob, int startIdx);
    int nullInputTrans(vector<MessageTuple*>& outMsgs,
                       bool& high_prob, int startIdx ) ;
    void restore(const StateSnapshot* snapshot);
    StateSnapshot* curState() ;
    virtual void reset() ;
    string getName() const { return "sync"; }
    
    void setMaster(const StateMachine* master) ;
    void addMachine(const StateMachine* mac) ;
    void addFailureGroup(const vector<const StateMachine*>& machines);
    
    static SyncMessage* setDeadline(MessageTuple* inMsg, int macid, int did);
    static SyncMessage* revokeDeadline(MessageTuple* inMsg, int macid, int did);
    
    static void setRecurring(int flag) { recurring_ = flag; }
protected:
    vector<int> _actives;
    // A vector indicates that which deadlines are active.
    // _ss_act[i] = 1 means that deadline i is active
    //              0                          inactive
    int _nextDl ;
    // The identifier of the deadline that is going to fire next
    // -1 indicates there is no deadline that will fire (no active deadline)
    int _time;
    // obsolete
    
    const int _numDl; // Total number of deadlines that will be used throughout the system
    const StateMachine* _masterPtr ;
    vector<MachineHandle> _allMacs;
    vector<FailureGroup> _failureGroups;
    
    int getNextActive();
    void failureEvent(size_t groupIdx, vector<MessageTuple*>& outMsgs);
    MessageTuple* generateMsg(const StateMachine *machine, const string& msg, bool isSet, int did);
};

class SyncMessage : public MessageTuple
{
public:
    /* Constructor
       set:
         If set = true, then this SyncMessage is a deadline setup message created by
         the master machine
         If set = false, then this SyncMessage is generated by Sync (machine) and should
         be distributed to the other machines in the system
       d: the identifier of the deadline to which this SyncMessage is associated
     **/
    
    SyncMessage(int src, int dest, int srcMsg, int destMsg, int subject, bool set, int d)
    :MessageTuple(src, dest, srcMsg, destMsg, subject), _isSet(set), _deadlineId(d) {}

    SyncMessage( const SyncMessage& msg )
    :MessageTuple(msg._src, msg._dest, msg._srcMsg, msg._destMsg, msg._subject)
    , _isSet(msg._isSet), _deadlineId(msg._deadlineId) {}
    
    ~SyncMessage() {}
    
    size_t numParams() { return 2; }
    int getParam(size_t arg) ;
    
    string toString() const;
    string toReadable() const { return toString(); }
    
    SyncMessage* clone() const { return new SyncMessage(*this) ; }
    
    // Produces the identifier of a received deadline, especially in the case when
    // there are multiple deadlines
    int get_num() { return _deadlineId; }
private:
    bool _isSet ;
    int _deadlineId ;
};


class SyncSnapshot: public StateSnapshot
{
    friend class Sync;
public:
    SyncSnapshot() {}
    SyncSnapshot( const SyncSnapshot& item );
    SyncSnapshot( const vector<int>& active, const int next, const vector<Sync::FailureGroup>& groups, const vector<Sync::MachineHandle>& handles, int time) ;
    ~SyncSnapshot() { }
    int curStateId() const ;
    string toString() const;
    string toReadable() const { return toString(); }
    int toInt() ;
    SyncSnapshot* clone() const { return new SyncSnapshot(*this) ; }

    int time() { return _ss_time; }

private:
    vector<int> _ss_act;
    int _ss_next ; 
    int _ss_time ;
    vector<bool> _ss_group_status;
    vector<bool> _ss_machine_status;
};

#endif

