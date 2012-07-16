#include <iostream>
#include <vector>
using namespace std;

#include "parser.h"
#include "fsm.h"
#include "pverify.h"

int main( int argc, char* argv[] ) 
{
    Parser* psrPtr = new Parser() ;
    psrPtr->declareMachine("transmitter");
    psrPtr->declareMachine("receiver");
    psrPtr->declareMachine("timer");
    psrPtr->declareMachine("comch");
    psrPtr->declareMachine("service");

    Fsm* xmitter = psrPtr->addFSM("transmitter.fsm");
    Fsm* rcvr = psrPtr->addFSM("receiver.fsm");
    Fsm* timer = psrPtr->addFSM("timer.fsm");
    Fsm* comch = psrPtr->addFSM("comch.fsm");    
    Fsm* serv = psrPtr->addFSM("service.fsm"); 

    ProbVerifier pvObj ;
    pvObj.addFsm(xmitter);
    pvObj.addFsm(rcvr);
    pvObj.addFsm(timer);
    pvObj.addFsm(comch);
    pvObj.addFsm(serv);

    pvObj.addRS(vector<int>(5,0));
    pvObj.start(5);

    system("Pause");

    return 0;
}

