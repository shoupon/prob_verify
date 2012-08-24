#include <iostream>
#include <vector>
#include <exception>
#include <stdexcept>
using namespace std;

#include "parser.h"
#include "fsm.h"
#include "pverify.h"
#include "define.h"

int main( int argc, char* argv[] ) 
{
    try {
        // Declare the names of component machines so as to register these names as id's in the parser
        Parser* psrPtr = new Parser() ;
        psrPtr->declareMachine("transmitter");
        psrPtr->declareMachine("receiver");
        psrPtr->declareMachine("timer");
#ifdef HALF_DUPLEX
        psrPtr->declareMachine("comch");
#endif
#ifdef FULL_DUPLEX
        psrPtr->declareMachine("fcomch");
        psrPtr->declareMachine("rcomch");
#endif
        psrPtr->declareMachine("service");

        // Create FSM objects from .fsm files
        Fsm* xmitter = psrPtr->addFSM("transmitter.fsm");
        Fsm* rcvr = psrPtr->addFSM("receiver.fsm");
        Fsm* timer = psrPtr->addFSM("timer.fsm");    
#ifdef HALF_DUPLEX
        Fsm* comch = psrPtr->addFSM("comch.fsm");
#endif
#ifdef FULL_DUPLEX
        Fsm* fcomch = psrPtr->addFSM("fcomch.fsm");
        Fsm* rcomch = psrPtr->addFSM("rcomch.fsm");
#endif
        Fsm* serv = psrPtr->addFSM("service.fsm"); 

        // Add the finite-state machines into ProbVerifier
        ProbVerifier pvObj ;
        pvObj.addMachine(xmitter);
        pvObj.addMachine(rcvr);
        pvObj.addMachine(timer);
#ifdef HALF_DUPLEX
        pvObj.addMachine(comch);
#endif
#ifdef FULL_DUPLEX
        pvObj.addMachine(fcomch);
        pvObj.addMachine(rcomch);
#endif    
        pvObj.addMachine(serv);

        // Specify the global states in the set RS
#ifdef HALF_DUPLEX
        vector<StateSnapshot*> rshalf;
        for( size_t i = 0 ; i < 5 ; ++i ) {
            StateSnapshot* snap = new FsmSnapshot(0);
            rshalf.push_back(snap);
        }
        vector<StateSnapshot*> rshalf2;
        for( size_t i = 0 ; i < 2 ; ++i ) {
            StateSnapshot* snap = new FsmSnapshot(3);
            rshalf2.push_back(snap);
        }
        for( size_t i = 2 ; i < 5 ; ++i ) {
            StateSnapshot* snap = new FsmSnapshot(0);
            rshalf2.push_back(snap);
        }
        pvObj.addRS(rshalf);
        pvObj.addRS(rshalf2);
#endif

#ifndef UN_NUM_ACK
        //int rs2[6] = {2,1,0,0,0,0};
#else
        //int rs2[6] = {3,3,0,0,0,0};
#endif

#ifdef FULL_DUPLEX
        vector<StateSnapshot*> rs1;
        for( size_t i = 0 ; i < 6 ; ++i ) {
            StateSnapshot* snap = new FsmSnapshot(0);
            rs1.push_back(snap);
        }
        vector<StateSnapshot*> rs2;
        for( size_t i = 0 ; i < 2 ; ++i ) {
            StateSnapshot* snap = new FsmSnapshot(3);
            rs2.push_back(snap);
        }
        for( size_t i = 2; i < 6 ; ++i ) {
            StateSnapshot* snap = new FsmSnapshot(0);
            rs2.push_back(snap);
        }
        pvObj.addRS(rs1);
        pvObj.addRS(rs2);
#endif        

        // Start the procedure of probabilistic verification. 
        // Specify the maximum probability depth to be explored
        pvObj.start(5);
    } catch( runtime_error& re ) {
        cerr << "Runtime error:" << endl 
             << re.what() << endl ;
    } catch (exception e) {        
        cerr << e.what() << endl;
    } catch (...) { 
        cerr << "Something wrong." << endl;
    }

    system("Pause");

    return 0;
}

