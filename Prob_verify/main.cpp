#include <iostream>
#include <vector>
#include <exception>
#include <stdexcept>
using namespace std;

#include "parser.h"
#include "fsm.h"
#include "pverify.h"
#include "define.h"
#include "./Lock/lock_utils.h"
#include "./Lock/controller.h"
#include "./Lock/lock.h"
#include "./Lock/competitor.h"
#include "./Lock/channel.h"

int main( int argc, char* argv[] )
{
    // class test
    /*
    {
        SeqCtrl sc(5) ;
        //cout << sc.isAllow(4) << endl ;
        cout << sc.engage(4) << endl ;
        //cout << sc.isAllow(3) << endl ;
        cout << sc.engage(3) << endl ;
        cout << sc.disengage(3) << endl ;
        cout << sc.disengage(3) << endl;
        cout << sc.isAllow(3) << endl;
        //cout << sc.engage(2) << endl ;
        
        cout << sc.disengage(3) << endl ;
        cout << sc.isAllow(3) << endl ;
        cout << sc.disengage(4) << endl;
        cout << sc.isAllow(3) << endl ;
        cout << sc.isAllow(4) << endl ;
        cout << sc.disengage(2) << endl ;
        cout << sc.isAllow(3) << endl ;
        cout << sc.isAllow(4) << endl ;
        cout << sc.engage(3) << endl ;
        cout << sc.engage(4) << endl ;
        cout << sc.disengage(4) << endl ;
        cout << sc.isAllow(2) << endl ;
        cout << sc.isAllow(4) << endl;
        cout << sc.engage(2) << endl ;
        cout << sc.disengage(2) << endl ;
        //cout << sc.engage(3) << endl ;
        cout << sc.disengage(4) << endl ;
        cout << sc.isAllow(2) << endl ;
        //cout << sc.engage(3) << endl;
        cout << sc.isAllow(4) << endl ;

        // the queue should be like [3,4,2] while only 3 is engaged at this point
        
        cout << sc.disengage(3) << endl ;
        
        // the queue should be emptied
        
        // Try invalid input
        cout << sc.engage(5) << endl; 
         
    }*/
    try {
        // Declare the names of component machines so as to register these names as id's in the parser
        Parser* psrPtr = new Parser() ;    
        /*
        for( int i = 0 ; i < 5 ; ++i ) {
            psrPtr->declareMachine( Lock_Utils::getLockName(i) );
            psrPtr->declareMachine( Lock_Utils::getCompetitorName(i) );
            for( int j = 0 ; j < 5 ; ++j ) {
                if( i == j )
                    continue;
                psrPtr->declareMachine( Lock_Utils::getChannelName(i,j) );
            }
        }*/

        // Create StateMachine objects
        int num = 5 ;
        int delta = 50; 
        Controller* ctrl = new Controller(psrPtr->getMsgTable(), psrPtr->getMacTable(),
                                          5, delta);
        vector<bool> active(num, false) ;
        active[3] = active[4] = true ;
        vector<vector<pair<int,int> > > nbrs(5);
        nbrs[3].push_back(make_pair(0,1)) ;
        nbrs[4].push_back(make_pair(1,2)) ;
        ctrl->setActives(active);
        ctrl->setNbrs(nbrs);
        
        vector<Lock*> arrLock ;
        vector<Competitor*> arrComp;
        for( size_t i = 0 ; i < num ; ++i )
            arrLock.push_back( new Lock((int)i,delta,num,psrPtr->getMsgTable(),
                                        psrPtr->getMacTable() ) );
        for( int i = 0 ; i < num ; ++i ) 
            arrComp.push_back( new Competitor(i,delta,num,psrPtr->getMsgTable(),
                                              psrPtr->getMacTable() ) );

        Channel* chan = new Channel(num, psrPtr->getMsgTable(),
                                    psrPtr->getMacTable() ) ;

        // Add the state machines into ProbVerifier
        ProbVerifier pvObj ;
        pvObj.addMachine(ctrl);
        for( size_t i = 0 ; i < arrLock.size() ; ++i )
            pvObj.addMachine(arrLock[i]);
        for( size_t i = 0 ; i < arrComp.size() ; ++i )
            pvObj.addMachine(arrComp[i]);
        pvObj.addMachine(chan);              
        
        // Specify the starting state
        GlobalState startPoint(pvObj.getMachinePtrs());
        startPoint.setParser(psrPtr);

        // Specify the global states in the set RS (stopping states)
        // initial state: FF
        StoppingState stopZero(&startPoint);
        stopZero.addAllow(new CompetitorSnapshot(-1,-1,-1,0), 9); // competitor 3
        stopZero.addAllow(new CompetitorSnapshot(-1,-1,-1,0), 10); // competitor 4
        pvObj.addRS(&stopZero);
        
        // state LF
        StoppingState stopLF(&startPoint);
        stopLF.addAllow(new LockSnapshot(1,-1,3,-1,1), 1) ;  // lock 0
        stopLF.addAllow(new LockSnapshot(1,-1,3,-1,1), 2) ;  // lock 1
        stopLF.addAllow(new LockSnapshot(1,-1,3,-1,1), 4) ;  // lock 3
        stopLF.addAllow(new CompetitorSnapshot(1,0,1,4), 9); // competitor 3
        pvObj.addRS(&stopLF);
        
        // state FL
        StoppingState stopFL(&startPoint);
        stopFL.addAllow(new LockSnapshot(1,-1,4,-1,1), 2) ;  // lock 1
        stopFL.addAllow(new LockSnapshot(1,-1,4,-1,1), 3) ;  // lock 2
        stopFL.addAllow(new LockSnapshot(1,-1,4,-1,1), 5) ;  // lock 4
        stopFL.addAllow(new CompetitorSnapshot(1,1,2,4), 10); // competitor 4
        pvObj.addRS(&stopFL);
        
        // Stopping states after message loss
        // state 3F
        StoppingState loss3F(&startPoint);
        loss3F.addAllow(new LockSnapshot(1,-1,3,-1,1), 1) ;  // lock 0
        loss3F.addAllow(new LockSnapshot(1,-1,3,-1,1), 2) ;  // lock 1
        loss3F.addAllow(new LockSnapshot(1,-1,3,-1,1), 4) ;  // lock 3
        loss3F.addAllow(new CompetitorSnapshot(1,0,1,3), 9); // competitor 3
        loss3F.addAllow(new ChannelSnapshot(), 11); // channel
        pvObj.addRS(&loss3F);
        
        // state 3'F
        StoppingState loss13F(&startPoint);
        //loss13F.addAllow(new LockSnapshot(1,-1,3,-1,1), 1) ;  // lock 0
        loss13F.addAllow(new LockSnapshot(1,-1,3,-1,1), 2) ;  // lock 1
        loss13F.addAllow(new LockSnapshot(1,-1,3,-1,1), 4) ;  // lock 3
        loss13F.addAllow(new CompetitorSnapshot(1,0,1,13), 9); // competitor 3
        loss13F.addAllow(new ChannelSnapshot(), 11); // channel
        pvObj.addRS(&loss13F);
        
        // state F3
        StoppingState lossF3(&startPoint);
        lossF3.addAllow(new LockSnapshot(1,-1,4,-1,1), 2) ;  // lock 1
        //lossF3.addAllow(new LockSnapshot(1,-1,4,-1,1), 3) ;  // lock 2
        lossF3.addAllow(new LockSnapshot(1,-1,4,-1,1), 5) ;  // lock 4
        lossF3.addAllow(new CompetitorSnapshot(1,1,2,3), 10); // competitor 4
        lossF3.addAllow(new ChannelSnapshot(), 11); // channel
        pvObj.addRS(&lossF3);
        
        // state F3'
        StoppingState lossF13(&startPoint);
        lossF13.addAllow(new LockSnapshot(1,-1,4,-1,1), 2) ;  // lock 1
        lossF13.addAllow(new LockSnapshot(1,-1,4,-1,1), 3) ;  // lock 2
        lossF13.addAllow(new LockSnapshot(1,-1,4,-1,1), 5) ;  // lock 4
        lossF13.addAllow(new CompetitorSnapshot(1,1,2,13), 10); // competitor 4
        lossF13.addAllow(new ChannelSnapshot(), 11); // channel
        pvObj.addRS(&lossF13);


        // Specify the error states
        StoppingState lock3FFree(&startPoint) ;
        lock3FFree.addAllow(new LockSnapshot(-1,-1,-1,-1,0), 1); // lock 0 in state 0
        lock3FFree.addAllow(new CompetitorSnapshot(2,0,1,4), 9); // competitor 3 in s4
        pvObj.addError(&lock3FFree);
        
        StoppingState lock3BFree(&startPoint) ;
        lock3BFree.addAllow(new LockSnapshot(-1,-1,-1,-1,0), 2); // lock 1 in state 0
        lock3BFree.addAllow(new CompetitorSnapshot(2,0,1,4), 9); // competitor 3 in s4
        pvObj.addError(&lock3BFree);
        
        StoppingState lock4FFree(&startPoint) ;
        lock4FFree.addAllow(new LockSnapshot(-1,-1,-1,-1,0), 2); // lock 1 in state 0
        lock4FFree.addAllow(new CompetitorSnapshot(2,1,2,4), 10); // competitor 4 in s4
        pvObj.addError(&lock4FFree);
        
        StoppingState lock4BFree(&startPoint) ;
        lock4BFree.addAllow(new LockSnapshot(-1,-1,-1,-1,0), 3); // lock 2 in state 0
        lock4BFree.addAllow(new CompetitorSnapshot(2,1,2,4), 10); // competitor 4 in s4
        pvObj.addError(&lock4BFree);

        StoppingState locklock(&startPoint) ;
        locklock.addAllow(new CompetitorSnapshot(2,0,1,4), 9); // competitor 3 in LOCK
        locklock.addAllow(new CompetitorSnapshot(2,1,2,4), 10) ; // competitor 4 in LOCK
        pvObj.addError(&locklock) ;

        // Start the procedure of probabilistic verification. 
        // Specify the maximum probability depth to be explored
        pvObj.start(5);

        // When complete, deallocate all machines
        delete ctrl ;
        for( size_t i = 0 ; i < arrLock.size() ; ++i ) 
            delete arrLock[i];
        for( size_t i = 0 ; i < arrComp.size() ; ++i )
            delete arrComp[i];
        delete chan ;                
        
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

