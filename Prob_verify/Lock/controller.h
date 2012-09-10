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
using namespace std;

#include "../statemachine.h"

class ControllerMessage;
class ControllerSnapshot ;

class Controller: public StateMachine
{
    typedef pair<int,int> Neighbor;
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
    void setActives(const vector<bool>& input) { if(input.size()==_actives.size()) _actives=input;}
private:
    const int _numVehs ;
    string _name;
    int _machineId;
    
    // _nbrs[i] contains the possible neighbors set of veh[i]
    vector<vector<Neighbor> > _nbrs ;    
    // when _actives[i] = true, vehicle i is able to initiate a merge
    //                   false, vehicle i cannot start a merge
    vector<bool> _actives;
    // The duration of the lock
    int _delta;

    // State variables
    // To record which vehicles are engaged in merge. The sequence of timeout for vehicles should be
    // the same as the sequence they are engaged in merge. The state of the controller can be
    // modeled as an FIFO queue as a consequence
    // the front of the queue is the zero-th element of the vector,
    // while the end of the queue is the last element
    vector<int> _engaged;
    // Indicates whether the competitor with the vector subscript is busy
    // negative value means idle. positive values record the starting timestamp of the lock
    vector<int> _busy;
    // The relative but not absolute time stamp for each event
    int _time ;
    // An alternating bit to check if any lock is successful. Initially with value 0, 
    // after a lock is completed it is changed into 1. The bit is changed to 0 after
    // another lock is completed. 
    int _altBit ;
    
};


class ControllerMessage: public MessageTuple
{
public:
    ControllerMessage(int src, int dest, int srcMsg, int destMsg, int subject)
    :MessageTuple(src,dest,srcMsg,destMsg,subject) {};

    ControllerMessage(const ControllerMessage& item)
    :MessageTuple(item)
    , _nParams(item._nParams), _front(item._front), _back(item._back), _timestamp(item._timestamp) {}

    ~ControllerMessage() {}
    
    size_t numParams() { return _nParams; }
    int getParam(size_t arg) ;        
    ControllerMessage* clone() { return new ControllerMessage(*this); }
    
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
    ControllerSnapshot(vector<int> eng, vector<int> busy, int time);
    ~ControllerSnapshot() {} 
    int curStateId() const ;
    // Returns the name of current state as specified in the input file
    string toString();
    // Cast the Snapshot into a integer. Used in HashTable
    int toInt() { return _hash_use ; }
    ControllerSnapshot* clone() const ;

private:
    vector<int> _ss_engaged;
    // Indicates whether the competitor with the vector subscript is busy
    // negative value means idle. positive values record the starting timestamp of the lock
    vector<int> _ss_busy;
    // The relative but not absolute time stamp for each event
    int _ss_time ;
    
    int _hash_use ;
};


#endif
