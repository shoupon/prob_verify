#include <cassert>

#include "statemachine.h"

Lookup StateMachine::machine_lookup_;
Lookup StateMachine::message_lookup_;

int StateMachine::transit(MessageTuple* in_msg,
                          vector<MessageTuple*>& out_msgs,
                          int& prob_level, int start_idx) {
  bool high_prob = true;
  int ret = this->transit(in_msg, out_msgs, high_prob, start_idx);
  if (high_prob)
    prob_level = 0;
  else
    prob_level = 1;
  return ret;
}

int StateMachine::transit(MessageTuple* in_msg,
                          vector<MessageTuple*>& out_msgs, int& prob_level) {
  return this->transit(in_msg, out_msgs, prob_level, 0);
}

int StateMachine::nullInputTrans(vector<MessageTuple*>& out_msgs,
                                 int& prob_level, int start_idx) {
  bool high_prob = true;
  int ret = this->nullInputTrans(out_msgs, high_prob, start_idx);
  if (high_prob)
    prob_level = 0;
  else
    prob_level = 1;
  return ret;
}

int StateMachine::nullInputTrans(vector<MessageTuple*>& out_msgs,
                                 int& prob_level) {
  return this->nullInputTrans(out_msgs, prob_level, 0);
}

bool MessageTuple::operator==(const MessageTuple &rhs) const
{
    if (_dest==rhs._dest &&
        _destMsg==rhs._destMsg && _subject==rhs._subject)
        return true;
    else
        return false;
}

bool MessageTuple::operator<(const MessageTuple &rhs) const
{
    if (_dest < rhs._dest)
        return true;
    else if(_dest > rhs._dest)
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

string StateSnapshot::toString() const {
  stringstream ss;
  ss << _ss_state ;
  return ss.str() ;
}
