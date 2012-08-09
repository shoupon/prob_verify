#include <vector>
#include <map>
using namespace std;

#ifndef TRANSITION_H
#define TRANSITION_H

typedef map<int,int>            OutLabelMap ;
typedef map<int,int>::iterator  OutLabelMapIter;
typedef pair<int,int>           OutLabel; // ToMachine.id ! message.id
typedef pair<int,int>           InLabel;  // FromMachine.id ? message.id

class Transition
{
private:
    int _from;
    int _input;
    
    InLabel _in;
    OutLabelMap _outs;
    
    bool _highProb; // TRUE: high probability transition, FALSE: otherwise

    string _labelName;
    bool _initialized ;

    int _machineID;
    int _id;

public:
    Transition(): _initialized(false) { }
    Transition(int from, int input, int to, int output, bool high, int macId = -1);
    bool ready() { return _initialized ; }
    bool addOutLabel(int to, int output); // addMessage()

    int getFromMachineId() { return _from; }
    int getInputMessageId() { return _input; }
    int getMachineId() { return _machineID; }
    int getId() { return _id; }
    bool isHigh() { return _highProb; }
    size_t getNumOutLabels() { return _outs.size() ; }
    OutLabel getOutLabel(size_t idx) { 
        OutLabelMapIter it = _outs.begin() ;
        for( size_t ii = 0 ; ii < idx ; ++ii )
            it++;            
        return *it; 
    }
    OutLabel getOutLabel() { return *(_outs.begin()) ; }
    string toString() ;

    void setId(int id) { this->_id = id; }

    bool operator == (const Transition& t) const ;
};

#endif