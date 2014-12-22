#include <cstring>

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
#include "fsm.h"
#include "lookup.h"

class Parser
{
private: 
  string _fileName;    
  Table _inputs;
  Table _outputs;
  Lookup* _messages;
  Lookup* _machineNames ;   

  vector<string> _listMacName;
  vector<string> _listMsg;

  int _lineNum ;
  stringstream _line;
  ifstream* _file;
  
  void getLine() ;
  void error(string message);
  Transition parseTransition(string& line, string& towardState);   
  bool addToTable(Table& tab, Entry& ent);
  bool separateLabel(const string& label, string& machine, string& message);    
  
  int findOrCreate(Table& tab, string key);
  int createMsg(string msg);
  int createMac(string macName);
  
  bool existInLabel(string label);
  bool existOutLabel(string label);
  bool existInTable(const Table& tab, string label);

public:
  Parser();
  ~Parser();
  Fsm* addFSM(string name);
  void declareMachine(string name);
  
  int messageToInt(string msg);
  int machineToInt(string macName);
  string IntToMessage(int id);
  string IntToMachine(int id);

  Lookup* getMsgTable() { return _messages; }
  Lookup* getMacTable() { return _machineNames;}
};

#endif
