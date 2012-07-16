#include <iostream>
#include <string>
#include <map>
using namespace std;

#include "transition.h"

Transition::Transition(int from, int input, int to, int output, bool high, int macId) 
:_from(from), _input(input), _initialized(true), _highProb(high), _machineID(macId)
{
    _outs.insert( OutLabel(to, output) );    
}

bool Transition::addOutLabel(int to, int output)
{
    OutLabelMap::iterator it = _outs.find(to);
    if( it == _outs.end() ) {
        _outs.insert( OutLabel(to,output) );
        return true;
    }
    else {
        return false;
    }
}

bool Transition::operator == (const Transition& t) const 
{
    if( _highProb != t._highProb ) 
        return false;

    if( _from == t._from && _input == t._input ) {
        // Check if the numbers of elements in OutLabelMap are the same
        if( _outs.size() != t._outs.size() )
            return false ;

        // For each element in OutLabelMap _outs, check if there is same output label in 't'
        size_t same = 0;   
        for( OutLabelMap::const_iterator it = _outs.begin() ; it != _outs.end() ; ++it ) {
            OutLabelMap::const_iterator tit = t._outs.find(it->first) ;
            if( tit == _outs.end() )
                return false ;
            else {
                if( tit->second == it->second )
                    same++;
            }                       
        }

        if( same == _outs.size() )
            return true ;
    }

    return false ;
}
        
