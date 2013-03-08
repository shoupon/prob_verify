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
#include "lookup.h"
#include "parser.h"

#pragma warning(disable : 4996)

Parser::Parser() 
{
    _messages = new Lookup();
    _machineNames = new Lookup();
    _messages->insert("nul");
    _machineNames->insert("nul");        
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
    //char* trimmed = new char[line.size()+1];
        
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
    
    _machineNames->insert(fsmName);
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
            _inputs.insert( Entry(inMsg, (int)_inputs.size()) ) ;
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
            _outputs.insert( Entry(outMsg, (int)_outputs.size()) ) ;
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
    Fsm* newMachine = new Fsm( string(fsmName), _messages, _machineNames );
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
        tab.insert( Entry(key, (int)tab.size()) );
        return (int) (tab.size()-1);
    }
    else
        return it->second;
}

int Parser::createMsg(string msg)
{
    int ret = _messages->insert(msg);
    return ret;    
}
   
int Parser::createMac(string macName)
{    
    int ret = _machineNames->insert(macName);
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
    _machineNames->insert(name);
}

int Parser::messageToInt(string msg)
{
    int ret = _messages->toInt(msg);

    if( ret == -1 ) {
        // msg is not in the lookup table
        ret = _messages->insert(msg);
    }
    
    return ret ;
}

int Parser::machineToInt(string macName)
{
    int ret = _machineNames->toInt(macName);

    if( ret == -1 ) {
        // machine name cannot be found in the lookup table
        ret = _machineNames->insert(macName);
    }

    return ret;
}

string Parser::IntToMessage(int id)
{
    return _messages->toString(id);   
}

string Parser::IntToMachine(int id)
{
    return _machineNames->toString(id);
}