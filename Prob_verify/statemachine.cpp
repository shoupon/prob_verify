#include <cassert>
using namespace std ;

#include "statemachine.h"


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