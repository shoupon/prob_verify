#include <cassert>

#include "statemachine.h"

Lookup* StateMachine::_msgLookup = 0 ;
Lookup* StateMachine::_macLookup = 0;

bool MessageTuple::operator==(const MessageTuple &rhs) const
{
    if (_src==rhs._src && _dest==rhs._dest &&
        _srcMsg==rhs._srcMsg && _destMsg==rhs._destMsg && _subject==rhs._subject)
        return true;
    else
        return false;
}

bool MessageTuple::operator<(const MessageTuple &rhs) const
{
    if (_src < rhs._src)
        return true;
    else if(_src > rhs._src)
        return false;
    if (_dest < rhs._dest)
        return true;
    else if(_dest > rhs._dest)
        return false;
    if (_srcMsg < rhs._srcMsg)
        return true;
    else if(_srcMsg > rhs._srcMsg)
        return false;
    if (_destMsg < rhs._destMsg)
        return true;
    else if (_destMsg > rhs._destMsg)
        return false;
    if (_subject < rhs._subject)
        return true;
    else
        return false;
}

int StateMachine::messageToInt(string msg) 
{ 
    int result = _msgLookup->toInt(msg);
    if( result != -1 ) {
        assert( result >= 0 ) ;
        return result;
    }
    else {
        return _msgLookup->insert(msg) ;       
    }
}

int StateMachine::machineToInt(string macName) 
{ 
    int result = _macLookup->toInt(macName);
    if( result != -1 ) {
        assert( result >= 0 ) ;
        return result;
    }
    else {
        return _macLookup->insert(macName) ;       
    }
}

string StateSnapshot::toString()
{
    stringstream ss;
    ss << _ss_state ;
    return ss.str() ;
}