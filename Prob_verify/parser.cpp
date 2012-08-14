#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <cassert>
#include <exception>
#include <stdexcept>
using namespace std;

#include "fsm.h"
#include "transition.h"

#include "parser.h"

#pragma warning(disable : 4996)

Parser::Parser() 
{
    _messages.insert( Entry("nul", 0) );
    _listMsg.push_back("nul");
    _machineNames.insert( Entry("nul", -1) );
}

// Read in a new line from the file and get rid of the semicolon at the end of line
void Parser::getLine() 
{
    // Read a new line from a file
    string tempLine;
    getline(*_file, tempLine);
        
    char* chLine = new char[tempLine.size()+1] ;

    // Copy c_str() to a newly allocated character array, so we can modify it 
    strcpy(chLine, tempLine.c_str() );
    
    // Get rid of the semicolon
    strtok (chLine,";");
    
    _line.clear();
    _line.str(string(chLine));
    
    _lineNum++;     

    delete[] chLine;
}

void Parser::error(string message)
{   
    stringstream ss ;
    ss << "In file " << _fileName << ": Line " << _lineNum << ": " << message << endl ;
    string str = ss.str();
    throw runtime_error(str);
}

Transition Parser::parseTransition(string& line, string& towardState)
{
    
    // N0 WHEN (UI_1?rel,UI_1!relcom) COST 1
    // N0 WHEN (UI_1?rel,UI_1!relcom) COST 1,
    char* trimmed = new char[line.size()+1];   
        
    stringstream ss ;
    string dump ;
    string label;
    int cost;
    string s_prob;
    ss << line ;

    // N0 WHEN (UI_1?rel,UI_1!relcom) COST 3 HIGH
    towardState.erase();
    ss >> towardState; // N0
    ss >> dump ; // WHEN
    ss >> label ; // (UI_1?rel,UI_1!relcom)
    ss >> dump ; // COST
    ss >> cost; // 3
    ss >> s_prob;
    
    /*
    if( sscanf(line.c_str(), "%s WHEN (%s,%s) COST %d", towardState, inMsg, outMsg, &cost) != 4 ) {
        error("Transition format incorrect!!") ;
        return Transition();
    }*/

    // The input message specified by its origin and its label
    char* chLabel = new char[label.size()+1];
    // The output message specified by its destination and its label    
    strcpy(chLabel, label.c_str());
    string inLabel = string(strtok(chLabel, "(,)" ));
    string outLabelCat = string(strtok(NULL, "(,)") );
    string fromMac;
    string inputMsg ;
    line = inLabel;
    // Check if the input label is already in parser's input list: this->_inputs
    if( !existInLabel(inLabel) ) {
        error("Input label has not been specified: " + inLabel) ;
        delete [] chLabel;
        return Transition();
    }
    // Separate message's origin and label
    if(!separateLabel(inLabel, fromMac, inputMsg)) {    
        error("Transition format incorrect!!") ;
        delete [] chLabel;
        return Transition();
    }    

    vector<string> tos;
    vector<string> outputs;
    // Separate the output message if there are multiple output messages by identifying '*' and
    // replace it by whitespace
    size_t found;
    found = outLabelCat.find_first_of("*");
    while (found!=string::npos) {
        outLabelCat[found]=' ';
        found=outLabelCat.find_first_of("*",found+1);
    }

    stringstream ssLabel;
    ssLabel.clear();
    ssLabel << outLabelCat;
    string outLabel ;    
    while (!ssLabel.eof())
    {        
        ssLabel >> outLabel;

        string toMac;
        string outputMsg; 
        // Check if the output label is already in parser's output list: this->_outputs
        if( !existOutLabel(outLabel) ) {
            error("Output label has not been specified: " + outLabel) ;
            delete [] chLabel;
            return Transition();
        }
        // Separate message's destination and label
        if(!separateLabel(outLabel, toMac, outputMsg)) {    
            error("Transition format incorrect!!") ;
            delete [] chLabel;
            return Transition();
        }
        tos.push_back(toMac);
        outputs.push_back(outputMsg); 
    }
    
    // Find the ID corresponding to human readable name
    int fromId = createMac( fromMac );        
    int inMsgId = createMsg( inputMsg );
    int toId = createMac( tos[0] );
    int outMsgId = createMsg( outputs[0] );

    // Determine the probability of transition
    bool prob;
    if( s_prob == "HIGH" || s_prob == "HIGH,")
        prob = true;
    else if( s_prob == "LOW" || s_prob == "LOW,")
        prob = false;
    else {
        error("Transition format incorrect, HIGH/LOW is expected") ;
        delete [] chLabel;
        return Transition();
    }

    // Create return object
    Transition ret = Transition(fromId, inMsgId, toId, outMsgId, prob);

    // Add more output messages if there are more than one
    assert(tos.size() == outputs.size());
    if( tos.size() > 1 ) {
        for( size_t i = 1 ; i < tos.size() ; ++i ) {
            toId = createMac( tos[i] );
            outMsgId = createMsg( outputs[i] );
            ret.addOutLabel(toId, outMsgId);
        }
    }

    delete [] chLabel;
    return ret;
}


Fsm* Parser::addFSM(string name)
{
    _lineNum = 0;
    _fileName = name;

    _file = new ifstream( name.c_str(), ios::in ) ;

    if( !(*_file) ) {
        error("Unable to open " + name );
        return 0;
    }
    
    string token;

    // Parse line of NAME
    this->getLine();
    char identifier[20];
    char fsmName[50];
    sscanf( _line.str().c_str(), "%s\t%s", identifier, fsmName );    
    
    if( string(identifier) != "NAME" ) {
        error( "Identifier 'NAME' is expected" );
        return 0 ;
    }

    InsertResult ir;
    ir = _machineNames.insert( Entry(string(fsmName), _machineNames.size()+1) ) ;
    /*
    if( ir.second == false ) {
        error( "Machine " + string(fsmName) + " has already been defined!" );
        return 0 ;
    }*/
    
    // Parse line of INPUTS
    char fromMachine[50];
    char* inLabel=0;
    this->getLine();
    _line >> token ;
    if( token != "INPUTS" ) {
        error( "Identifier 'INPUT' is expected!!" );
        return 0 ;
    }

    while(!_line.eof()) {
        _line >> token ;

        strcpy(fromMachine, token.c_str() );
        strtok(fromMachine, "?");
        inLabel = strtok(NULL, "?,");  
        if( inLabel == 0 )
            cout << "DEBUG" << endl ;
        else if( *inLabel == 0 ) {
            error( "INPUTS format incorrect!!" );
            return 0 ;
        }
        else {   
            // TODO: Need to check for duplicate input or message
            string inMsg = string(fromMachine)+"?"+inLabel;
            _inputs.insert( Entry(inMsg, _inputs.size()) ) ;
            createMsg(inLabel);
            //_messages.insert( Entry(string(inLabel), _messages.size()) );
        }
    }

    // Parse line of OUTPUTS
    char toMachine[50] ;
    char* outLabel = 0;
    this->getLine();
    _line >> token ;
    if( token != "OUTPUTS" ) {
        error( "Identifier 'OUTPUT' is expected!!" );
        return 0 ;
    }

    while(!_line.eof()) {
        _line >> token ;

        strcpy(toMachine, token.c_str() );
        strtok(toMachine, "!");
        outLabel = strtok(NULL, "!,");      
        if( *outLabel == 0 ) {
            error( "OUTPUT format incorrect!!" );
            return 0 ;
        }
        else {   
            // TODO: Need to check for duplicate input or message
            string outMsg = string(toMachine)+"!"+outLabel;
            _outputs.insert( Entry(outMsg, _outputs.size()) ) ;
            createMsg(outLabel);
            //_messages.insert( Entry(string(outLabel), _messages.size()) );
        }
    }

    // Parse line of INITIAL_STATE    
    this->getLine();
    _line >> token ;

    if( token != "INITIAL_STATE" ) {
        error( "Identifier 'INITIAL_STATE' is expected!!" );
        return 0 ;
    }

    _line >> token ;
    Fsm* newMachine = new Fsm( string(fsmName) );
    newMachine->addState(token);

    if( !_line.eof() ) {
        error("More than one initial state!!");
        delete newMachine;
        return 0;
    }

    // Parse line of OTHER_STATES    
    this->getLine();
    _line >> token;
    if( token != "OTHER_STATES" ) {
        error( "Identifier 'OTHER_STATES' is expected!!" );
        return 0 ;
    }

    char stateName[50];
    while(!_line.eof()) {
        _line >> token ;
        strcpy(stateName,token.c_str());
        strtok(stateName, ","); 
        newMachine->addState( string(stateName) );
    }

    // Parse line of TRANSITIONS
    (*_file) >> token ;
    if( token != "TRANSITIONS" ) {
        error("Identifier 'TRANSITIONS' is expected!!");
        delete newMachine;
        return 0;
    }
    
    string tail, head; 
    while(1) {
        this->getLine();
        _line >> token ;

        if( token == "EOF" )
            break ;

        if( token != ":" ) { // Start of new tail state
            tail = token ;
            _line >> token ;
        }
        
        stringstream ss;
        while(!_line.eof()) {
            _line >> token ;
            ss << token << " ";
        }

        string transLabel = ss.str();
        Transition edge = parseTransition(transLabel, head);
        if( !edge.ready() ) {
            error("Error in transition format!!");
            delete newMachine;
            return 0;
        }

        if( !newMachine->addTransition(tail, string(head), edge) ) {
            error("There is already the same input label " + transLabel + " for state " + tail);
            delete newMachine;
            return 0;
        }
    }

    return newMachine;
}
      
bool Parser::addToTable(Table& tab, Entry& ent)
{
    if( tab.find(ent.first) != tab.end() )
        return false ;
    else {
        tab.insert(ent);
        return true;
    }
}

// Separate input or output labels in the form of fromMachine?inputMsg or toMachine!outputMsg
// TODO

bool Parser::separateLabel(const string& label, string& machine, string& message){
    char* charLabel = new char[label.size()+1];
    
    strcpy(charLabel, label.c_str());
    char* charMac = strtok(charLabel, "?!");
    char* charMsg = strtok(NULL, "?!");

    machine = string(charMac);
    if( charMsg != 0 )
        message = string(charMsg);
    else {
        message = machine;
    }

    delete [] charLabel;

    if( message.size()*machine.size() == 0 )
        return false;

    return true;
}

int Parser::findOrCreate(Table& tab, string key)
{
    Table::iterator it = tab.find(key);
    if( it == tab.end() ) {
        tab.insert( Entry(key, tab.size()) );
        return tab.size()-1;
    }
    else
        return it->second;
}

int Parser::createMsg(string msg)
{
    int ret = findOrCreate(this->_messages, msg);
    if(  (size_t)ret >= _listMsg.size() ) {
        // if the return idx >= the size of msg vector, which means the message is created, 
        // a new entry in msg vector should be created, too
        _listMsg.push_back(msg);
    }

    return ret;
}
   
int Parser::createMac(string macName)
{
    // Search macName in the Table(map) _machineNames
    // If the returned value == -1 or < size of list of machine names, the macName exists in the table
    int ret = findOrCreate(this->_machineNames, macName) ;
    
    if( ret == -1 ) // macName == "nul", no need to create new entry
        return ret;

    if( (size_t)ret >= _listMacName.size() ) {
        // If returned value is out of the range of the list, a new entry should be created at the
        // back of ths list
        _listMacName.push_back(macName);
    }
    return ret;
}

bool Parser::existInLabel(string label)
{
    return existInTable(_inputs, label);
}

bool Parser::existOutLabel(string label)
{
    return existInTable(_outputs, label);
}

bool Parser::existInTable(const Table& tab, string label)
{
    if( label == "nul" )
        return true;

    return tab.find(label) != tab.end();
}

void Parser::declareMachine(string name) 
{ 
    if( _machineNames.find( name ) == _machineNames.end() ) {
        assert(_listMacName.size() == _machineNames.size()-1 );

        _machineNames.insert( Entry(name, _machineNames.size()-1) );         
        _listMacName.push_back(name);
    }
}

int Parser::messageToInt(string msg)
{
    Table::iterator it = this->_messages.find(msg);
    if( it == _messages.end() ) {
        _messages.insert( Entry(msg, _messages.size() - 1) );
        _listMsg.push_back(msg);
        return _messages.size() - 1;
    }
    else {
        return it->second;
    }
}

int Parser::machineToInt(string macName)
{
    Table::iterator it = this->_machineNames.find(macName);
    if( it == _machineNames.end() ) {
        _machineNames.insert( Entry(macName, _machineNames.size() - 1) );
        _listMacName.push_back(macName);
        return _machineNames.size() - 1;
    }
    else {
        return it->second;
    }
}

string Parser::IntToMessage(int id)
{
    if( id < 0 ) {
        throw runtime_error("There is no message associated with id < 0, error must present");
    }
    else if( (size_t)id >= _listMsg.size() ) {
        stringstream ss ;
        ss << "Unable to find message corresponding to the id = " ;
        ss << id ;
        throw runtime_error(ss.str());
    }
    else {
        return _listMsg[id];
    }
}

string Parser::IntToMachine(int id)
{
    if( id < 0 ) {
        if( id == -1 )
            return string("nul");
        else {
            throw runtime_error("There is no machine with id < -1, error must present");
        }
    }
    else if( (size_t)id >= _listMacName.size() ) {
        stringstream ss ;
        ss << "Unable to find machine corresponding to the id = " ;
        ss << id ;
        throw runtime_error(ss.str());
    }
    else {
        return _listMacName[id];
    }
}