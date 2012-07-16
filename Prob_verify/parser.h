#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
using namespace std;

#ifndef PARSER_H
#define PARSER_H

#include "define.h"
#include "fsm.h"
#include "state.h"
#include "transition.h"

class Parser
{
private: 
    string _fileName;
    Table _machineNames ;   
    Table _inputs;
    Table _outputs;
    Table _messages;

    int _lineNum ;
    stringstream _line;
    ifstream* _file;
    
    void getLine() ;
    void error(string message);
    Transition parseTransition(string& line, string& towardState);   
    bool addToTable(Table& tab, Entry& ent);
    bool separateLabel(const string& label, string& machine, string& message);    
    int findOrCreate(Table& tab, string key);
    
    bool existInLabel(string label);
    bool existOutLabel(string label);
    bool existInTable(const Table& tab, string label);
public:
    Parser();
    Fsm* addFSM(string name);
    void declareMachine(string name);
};

#endif