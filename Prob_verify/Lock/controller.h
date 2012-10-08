//
//  lock_service.h
//  Prob_verify
//
//  Created by Shou-pon Lin on 8/27/12.
//  Copyright (c) 2012 Shou-pon Lin. All rights reserved.
//

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <string>
#include <vector>
#include <queue>
#include <bitset>
#include <iostream>
using namespace std;

#include "../statemachine.h"
#include "../define.h"

class ControllerMessage;
class ControllerSnapshot ;

class SeqCtrl
{
    vector<int> _seq;
    vector<int> _inQueue ;
    vector<int> _engaged;
    
    const int _num;
public:
    SeqCtrl(int num):_num(num)
    {
        _inQueue.resize(num,0);
        _engaged.resize(num,0);
        _seq.reserve(num);
        //cout << "SeqCtrl created" << endl ;
    }
    SeqCtrl(const SeqCtrl& item)
    :_num(item._num)
    {
        copyVec(item._inQueue, _inQueue);
        copyVec(item._engaged, _engaged);
        
        _seq.resize(item._seq.size());
        for( size_t i = 0 ; i < _seq.size() ; ++i ) {
            _seq[i] = item._seq[i];
        }
        //_seq = vector<int>(item._seq) ;
        //cout << "SeqCtrl created" << endl ;
    }
    
    void reset()
    {
        _inQueue.resize(_num,0);
        _engaged.resize(_num,0);
        while( !_seq.empty())
            _seq.erase(_seq.begin()) ;
    }
    bool isAllow(int veh)
    {
        // check if vehicle id is valid
        if( !isValid(veh) )
            return false;
        
        if( _engaged[veh] == 1 )
            return false;
        
        if( _inQueue[veh] == 0 )
            return true ;
        else {
            if( _seq.front() == veh )
                return true ;
            else
                return false;
        }
    }
    bool engage(int veh)
    {
        if( !isValid(veh) )
            return false;
        
        // this function can be called only when the machine is allowed to engage
        assert( isAllow(veh) ) ;
        
        _engaged[veh] = 1 ;
        _inQueue[veh] = 1 ;
        _seq.push_back(veh);
        
        return true;
    }
    bool disengage(int veh)
    {
        if( !isValid(veh) )
            return false ;
        
        if( _engaged[veh] == 0 )
            return false;
        
        _engaged[veh] = 0 ;
        while( !_seq.empty() ) {
            int front = _seq.front();
            assert( isValid(front) ) ;
            if( _engaged[front] == 0 ) {
                _inQueue[front] = 0 ;
                _seq.erase(_seq.begin()) ;
            }
            else {
                break;
            }
        }
        
        return true ;
    }
    
    int getFront() { if( _seq.size()>0) return _seq[0] ; else return -1;}
    
private:
    bool isValid(int veh)
    {
        if( veh >= _num || veh < 0 ) {
            cout << "The input to SeqCtrl is not valid" << endl;
            return false ;
        }
        else
            return true;
    }
    
    template <typename T>
    void copyVec(const vector<T>& origin, vector<T>& dest) {
        dest.resize(origin.size());
        for( size_t i = 0 ; i < dest.size() ; ++i ) {
            dest[i] = origin[i];
        }
    }
};

class Controller: public StateMachine
{
    typedef pair<int,int> Neighbor;
    SeqCtrl* _seqReg ;
public:
    Controller( Lookup* msg, Lookup* mac, int num, int delta ) ;        
    int transit(MessageTuple* inMsg, vector<MessageTuple*>& outMsgs,
                bool& high_prob, int startIdx ) ;   
    int nullInputTrans(vector<MessageTuple*>& outMsgs,
                       bool& high_prob, int startIdx ) ;   
    void restore(const StateSnapshot* snapshot) ;
    StateSnapshot* curState() ;   
    void reset() ;

    // Generate all possible sets of neighbors
    void allNbrs();
    // Specify the possible neighbors that are being tested
    void setNbrs(vector<vector<Neighbor> >& nbrs) { _nbrs = nbrs; }
    // Specify the competitors that will be tested; The default is true for all vehicles
    void setActives(const vector<bool>& input) ;
private:
    const int _numVehs ;
    string _name;
    int _machineId;
    
    // _nbrs[i] contains the possible neighbors set of veh[i]
    vector<vector<Neighbor> > _nbrs ;    
    // when _actives[i] = true, vehicle i is able to initiate a merge
    //                   false, vehicle i cannot start a merge
    vector<bool> _actSetting;
    vector<bool> _actives;
    // The duration of the lock
    int _delta;

    // State variables
    // To record which vehicles are engaged in merge. The sequence of timeout for vehicles
    // should be the same as the sequence they are engaged in merge. The state of the
    // controller can be modeled as an FIFO queue as a consequence the front of the queue is
    // the zero-th element of the vector, while the end of the queue is the last element
    // A vehicle identifier will be removed of _engaged only when itself and the locks it
    // requested are all released and reset
    vector<int> _engaged;
    // Indicates whether the competitor with the vector subscript is busy
    // negative value means idle. positive values record the starting timestamp of the lock
    vector<int> _busy;
    // Store the competitor that is associated with this lock. The indices of the arrays
    // are the identifier of the lock. For example, consider competitor 3 wants to merge
    // between lock 0 and lock 1. _fronts[0]=3, _back[1]=3, _selves[3]=3.
    // --> This doesn't work. Two competitors competing for the same lock attempt to write
    //     to the same memory. (Similar to the behavior in Fischer's, but this time
    //     without protection) For instance, comp3 and comp4 write to lock1. This will lead
    //     to undefined behavior, leading to assertion failure
    // Indices associated with the competitor instead. Use to take records of the locks
    // a competitor is collaborating with. ex: comp3 merging between lock0 and lock1. Then
    // _fronts[3]=0, _backs[3]=1, _selves[3]=3
    // Along with _busy, are used to keep track of which locks are not being released, and
    // which locks to send the timeout message
    vector<int> _fronts;
    vector<int> _backs;
    vector<int> _selves;
    // The relative but not absolute time stamp for each event
    int _time ;
    // An alternating bit to check if any lock is successful. Initially with value 0, 
    // after a lock is completed it is changed into 1. The bit is changed to 0 after
    // another lock is completed. 
    int _altBit ;
    
    // Make sure all machines involved in a merge (1 competitor + 3 locks) are either
    // released or timed out, then remove the id of such vehicle of _engaged queue.
    bool removeEngaged(int i);
//    bool can
};


class ControllerMessage: public MessageTuple
{
public:
    ControllerMessage(int src, int dest, int srcMsg, int destMsg, int subject)
    :MessageTuple(src,dest,srcMsg,destMsg,subject) {};
    
    ControllerMessage(int src, int dest, int srcMsg, int destMsg, int subject, int master)
    :MessageTuple(src,dest,srcMsg,destMsg,subject), _timestamp(master) {};

    ControllerMessage(const ControllerMessage& item)
    :MessageTuple(item)
    , _nParams(item._nParams), _front(item._front), _back(item._back), _timestamp(item._timestamp) {}

    ~ControllerMessage() {}
    
    size_t numParams() { return _nParams; }
    int getParam(size_t arg) ;        
    ControllerMessage* clone() const { return new ControllerMessage(*this); }
    
    void addParams(int time, int front, int back);   
protected:
    int _nParams;
    int _front;
    int _back;
    int _timestamp;
};

class ControllerSnapshot: public StateSnapshot
{
    friend class Controller;
public:
    ControllerSnapshot(vector<int> eng, vector<int> busy,
                       vector<int> front, vector<int> back, vector<int> self, int time,
                       const SeqCtrl* seqCtrl);
    ControllerSnapshot(const ControllerSnapshot& item);
    ~ControllerSnapshot() { delete _ss_seqCtrl ;}
    int curStateId() const ;
    // Returns the name of current state as specified in the input file
    string toString();
    // Cast the Snapshot into a integer. Used in HashTable
    int toInt() { return _hash_use ; }
    ControllerSnapshot* clone() const ;

private:
    // Keep track of the sequence of merge initiation
    vector<int> _ss_engaged;
    // Indicates whether the competitor with the vector subscript is busy
    // negative value means idle. positive values record the starting timestamp of the lock
    vector<int> _ss_busy;
    vector<int> _ss_front;
    vector<int> _ss_back;
    vector<int> _ss_self;
    // Pointer to a SequenceController
    SeqCtrl* _ss_seqCtrl;
    // The relative but not absolute time stamp for each event
    int _ss_time ;
    
    int _hash_use ;
};


#endif
