#include <iostream>
#include <vector>
#include <string>
using namespace std;

#include "state.h"
#include "transition.h"

bool State::operator==(const State &other) const 
{
    if( this->_stateID == other._stateID )
        return true ;
    else
        return false;
}

void State::match(int sndrID, int msgID, State* rcvrStatePtr, vector<EdgeProb>& result)
{
    vector<int> nextId;
    vector<bool> nextProb;
    vector<Transition> nextTrans;
    rcvrStatePtr->getNextState(sndrID, msgID, nextId, nextProb, nextTrans );

    result.clear();
    for( size_t ii = 0 ; ii < nextId.size() ; ++ii ) 
        result.push_back(  EdgeProb(rcvrStatePtr->_nexts[nextId[ii]], nextProb[ii])  );
}

void State::receive(int from, int message, vector<Arrow>& matched )
{
    matched.clear();
    for( size_t ii = 0 ; ii < _trans.size() ; ++ii ) {
        if( _trans[ii].getFromMachineId() == from 
                && _trans[ii].getInputMessageId() == message ) {            
            matched.push_back( Arrow(ii, this->_nexts[ii]) );  
        }
    }    
}

void State::addOutEdge(Transition edge, State* nextState )
{
    edge.setId(_trans.size());
    _trans.push_back(edge);
    _nexts.push_back(nextState);
}

bool State::findTransition (const Transition& label) const
{
    for( size_t ii = 0 ; ii < _trans.size() ; ++ii ) {
        if( _trans[ii] == label )
            return true;
    }

    return false;
}

void State::getNextState(int from, int input, vector<int>& nexts, 
                         vector<bool>& prob_high, vector<Transition>& trans) 
{
    nexts.clear();
    prob_high.clear();
    for( size_t ii = 0 ; ii < _trans.size() ; ++ii ) {
        if( _trans[ii].getFromMachineId() == from )
            if( _trans[ii].getInputMessageId() == input ) {
                nexts.push_back( _nexts[ii]->getID() );
                prob_high.push_back(_trans[ii].isHigh() );
                trans.push_back(_trans[ii]);
            }
    } 
}

int State::isActive() 
{
    for( size_t ii = 0 ; ii < _actives.size(); ++ii ) 
        if( _actives[ii] )
            return ii;
    return -1 ;
}