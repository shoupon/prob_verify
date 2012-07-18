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
        pvObj.addFsm(xmitter);
        pvObj.addFsm(rcvr);
        pvObj.addFsm(timer);
#ifdef HALF_DUPLEX
        pvObj.addFsm(comch);
#endif
#ifdef FULL_DUPLEX
        pvObj.addFsm(fcomch);
        pvObj.addFsm(rcomch);
#endif    
        pvObj.addFsm(serv);

        // Specify the global states in the set RS
#ifdef HALF_DUPLEX
        pvObj.addRS(vector<int>(5,0));
#endif
#ifdef FULL_DUPLEX
        pvObj.addRS(vector<int>(6,0));
#endif        

        // Start the procedure of probabilistic verification. 
        // Specify the maximum probability depth to be explored
        pvObj.start(5);
    } catch (exception e) {
        cerr << e.what() << endl;
    }

    system("Pause");

    return 0;
}

