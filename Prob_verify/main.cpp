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
        int num = 7 ;
        int delta = 50; 
        Controller* ctrl = new Controller(psrPtr->getMsgTable(), psrPtr->getMacTable(),
                                          num, delta);
        vector<bool> active(num, false) ;
        active[3] = active[5] = active[6] = true ;
        vector<vector<pair<int,int> > > nbrs(num);
        nbrs[3].push_back(make_pair(1,2)) ;
        nbrs[5].push_back(make_pair(2,4)) ;
        nbrs[6].push_back(make_pair(1,2)) ;
        ctrl->setActives(active);
        ctrl->setNbrs(nbrs);
        
        vector<Lock*> arrLock ;
        for( size_t i = 0 ; i < num ; ++i )
            arrLock.push_back( new Lock((int)i,delta,num,psrPtr->getMsgTable(),
                                        psrPtr->getMacTable() ) );
        Channel* chan = new Channel(num, psrPtr->getMsgTable(),
                                    psrPtr->getMacTable() ) ;

        // Add the state machines into ProbVerifier
        ProbVerifier pvObj ;
        pvObj.addMachine(ctrl);
        for( size_t i = 0 ; i < arrLock.size() ; ++i )
            pvObj.addMachine(arrLock[i]);
        pvObj.addMachine(chan);              
        
        // Specify the starting state
        GlobalState startPoint(pvObj.getMachinePtrs());
        startPoint.setParser(psrPtr);

        // Specify the global states in the set RS (stopping states)
        // initial state: FF
        StoppingState stopZero(&startPoint);
        stopZero.addAllow(new LockSnapshot(-1,-1,-1,-1,0), 1); // lock 0
        stopZero.addAllow(new LockSnapshot(-1,-1,-1,-1,0), 2); // lock 1
        stopZero.addAllow(new LockSnapshot(-1,-1,-1,-1,0), 3); // lock 2
        stopZero.addAllow(new LockSnapshot(-1,-1,-1,-1,0), 4); // lock 3
        stopZero.addAllow(new LockSnapshot(-1,-1,-1,-1,0), 5); // lock 4
        stopZero.addAllow(new LockSnapshot(-1,-1,-1,-1,0), 6); // lock 5
        stopZero.addAllow(new LockSnapshot(-1,-1,-1,-1,0), 7); // lock 6
        stopZero.addAllow(new ChannelSnapshot(), 8); // channel
        pvObj.addRS(&stopZero);
        
        // state LF
        StoppingState stopLF(&startPoint);
        stopLF.addAllow(new LockSnapshot(-1,-1,-1,-1,0), 1); // lock 0
        stopLF.addAllow(new LockSnapshot(10,-1,-1,3,1), 2); // lock 1
        stopLF.addAllow(new LockSnapshot(10,-1,-1,3,1), 3); // lock 2
        stopLF.addAllow(new LockSnapshot(10,1,2,-1,4), 4); // lock 3
        //stopLF.addAllow(new ChannelSnapshot(), 8); // channel
        pvObj.addRS(&stopLF);
        
        
        // state FL
        StoppingState stopFL(&startPoint);
        stopFL.addAllow(new LockSnapshot(-1,-1,-1,-1,0), 1); // lock 0
        stopFL.addAllow(new LockSnapshot(10,-1,-1,5,1), 3); // lock 2
        stopFL.addAllow(new LockSnapshot(10,-1,-1,5,1), 5); // lock 4
        stopFL.addAllow(new LockSnapshot(10,2,4,-1,4), 6); // lock 5
        //stopFL.addAllow(new ChannelSnapshot(), 8); // channel
        pvObj.addRS(&stopFL);
        
        // state FFL
        StoppingState stopFFL(&startPoint);
        stopFFL.addAllow(new LockSnapshot(-1,-1,-1,-1,0), 1); // lock 0
        stopFFL.addAllow(new LockSnapshot(10,-1,-1,6,1), 2); // lock 1
        stopFFL.addAllow(new LockSnapshot(10,-1,-1,6,1), 3); // lock 2
        stopFFL.addAllow(new LockSnapshot(10,1,2,-1,4), 7); // lock 6
        //stopFFL.addAllow(new ChannelSnapshot(), 8); // channel
        pvObj.addRS(&stopFFL);
        
        /*
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
         */

        // Specify the error states
        StoppingState lock3FFree(&startPoint) ;
        lock3FFree.addAllow(new LockSnapshot(-1,-1,-1,-1,0), 2); // lock 1 in state 0
        lock3FFree.addAllow(new LockSnapshot(10,1,2,-1,4), 4); // lock 3 in state 4
        pvObj.addError(&lock3FFree);
        
        StoppingState lock3BFree(&startPoint) ;
        lock3BFree.addAllow(new LockSnapshot(-1,-1,-1,-1,0), 3); // lock 2 in state 0
        lock3BFree.addAllow(new LockSnapshot(10,1,2,-1,4), 4); // lock 3 in state 4
        pvObj.addError(&lock3BFree);
        
        StoppingState lock5FFree(&startPoint) ;
        lock5FFree.addAllow(new LockSnapshot(-1,-1,-1,-1,0), 3); // lock 2 in state 0
        lock5FFree.addAllow(new LockSnapshot(10,2,4,-1,4), 6); // lock 5 in state 4
        pvObj.addError(&lock5FFree);
        
        StoppingState lock5BFree(&startPoint) ;
        lock5BFree.addAllow(new LockSnapshot(-1,-1,-1,-1,0), 5); // lock 4 in state 0
        lock5BFree.addAllow(new LockSnapshot(10,2,4,-1,4), 6); // lock 5 in state 4
        pvObj.addError(&lock5BFree);
        
        StoppingState lock6FFree(&startPoint) ;
        lock6FFree.addAllow(new LockSnapshot(-1,-1,-1,-1,0), 2); // lock 1 in state 0
        lock6FFree.addAllow(new LockSnapshot(10,1,2,-1,4), 7); // lock 6 in state 4
        pvObj.addError(&lock6FFree);
        
        StoppingState lock6BFree(&startPoint) ;
        lock6BFree.addAllow(new LockSnapshot(-1,-1,-1,-1,0), 3); // lock 2 in state 0
        lock6BFree.addAllow(new LockSnapshot(10,1,2,-1,4), 7); // lock 6 in state 4
        pvObj.addError(&lock6BFree);
         
        StoppingState lock35(&startPoint) ;
        lock35.addAllow(new LockSnapshot(10,1,2,-1,4), 4); // lock 3 in state 4
        lock35.addAllow(new LockSnapshot(10,2,4,-1,4), 6); // lock 5 in state 4
        pvObj.addError(&lock35) ;
        
        StoppingState lock36(&startPoint) ;
        lock36.addAllow(new LockSnapshot(10,1,2,-1,4), 4); // lock 3 in state 4
        lock36.addAllow(new LockSnapshot(10,1,2,-1,4), 7); // lock 6 in state 4
        pvObj.addError(&lock35) ;
        
        StoppingState lock56(&startPoint) ;
        lock56.addAllow(new LockSnapshot(10,2,4,-1,4), 6); // lock 5 in state 4
        lock56.addAllow(new LockSnapshot(10,1,2,-1,4), 7); // lock 6 in state 4
        pvObj.addError(&lock35) ;
         

        // Start the procedure of probabilistic verification. 
        // Specify the maximum probability depth to be explored
        pvObj.start(5);

        // When complete, deallocate all machines
        delete ctrl ;
        for( size_t i = 0 ; i < arrLock.size() ; ++i ) 
            delete arrLock[i];
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

