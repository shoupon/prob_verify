#include <cassert>

#include "statemachine.h"

//Lookup* StateMachine::_msgLookup = 0 ;
//Lookup* StateMachine::_macLookup = 0;
Lookup StateMachine::machine_lookup_;
Lookup StateMachine::message_lookup_;

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

MessageTuple* StateMachine::createOutput(MessageTuple* in_msg,
                                         int dest, int dest_msg) {
  if (in_msg)
    return new MessageTuple(in_msg->srcID(), dest,
                            in_msg->srcMsgId(), dest_msg, macId());
  else
    return new MessageTuple(0, dest, 0, dest_msg, macId());
}

MessageTuple* StateMachine::createOutput(MessageTuple* in_msg,
                                         const string& dest_name,
                                         const string& dest_msg_name) {
  return createOutput(in_msg,
                      machineToInt(dest_name), messageToInt(dest_msg_name));
}

int tableLookup(Lookup& tab, const string& msg) {
  int result = tab.toInt(msg);
  if (result != -1) {
    assert(result >= 0);
    return result;
  }
  else {
    return tab.insert(msg);
  }
}

int StateMachine::messageToInt(const string& msg) {
  return tableLookup(message_lookup_, msg);
}

int StateMachine::machineToInt(const string& machine_name) {
  return tableLookup(machine_lookup_, machine_name);
}

string StateMachine::IntToMessage(int num) {
  return message_lookup_.toString(num);
}

string StateMachine::IntToMachine(int num) {
  return machine_lookup_.toString(num);
}

void StateMachine::dumpMachineTable() {
  machine_lookup_.dump();
}

string StateSnapshot::toString()
{
    stringstream ss;
    ss << _ss_state ;
    return ss.str() ;
}
