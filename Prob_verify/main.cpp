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

        // Specify the global states in the set RS (stopping states)
        // initial state: FF
        vector<StateSnapshot*> rs;
        // controller snapshot
        vector<int> engaged ;
        vector<int> busy(num,-1) ;
        SeqCtrl sc(num);
        StateSnapshot* cSnap = new ControllerSnapshot(engaged,busy,busy,busy,busy,0,&sc);
        rs.push_back(cSnap);
        // lock snapshots
        for( int i = 0 ; i < num; ++i ) {
            // LockSnapshot(int ts, int t2, int oldCom, int newCom, int state)
            StateSnapshot* lSnap = new LockSnapshot(-1,-1,-1,-1,0);
            rs.push_back(lSnap);
        }
        // competitor snapshots
        for( int i = 0 ; i < num ; ++i ) {
            //CompetitorSnapshot(int t, int front, int back,int state)
            StateSnapshot* compSnap = new CompetitorSnapshot(-1,-1,-1,0);
            rs.push_back(compSnap);
        }
        // channel snapshots
        StateSnapshot* chanSnap = new ChannelSnapshot();
        rs.push_back(chanSnap);                                                              
        pvObj.addRS(rs);
        
        // state LF
        vector<StateSnapshot*> rsLF = rs ;
        engaged.push_back(3);
        vector<int> front = busy ;
        vector<int> back = busy ;
        vector<int> self = busy ;
        busy[3] = 1;
        front[0] = 1;
        back[1] = 1;
        self[3] = 1;
        // 35 arbitrarily picked. (Only engaged matters)
        cSnap = new ControllerSnapshot(engaged,busy,front,back,self,35,&sc);
        rsLF[0] = cSnap ; // controller
        rsLF[1] = new LockSnapshot(1,-1,3,-1,1); // lock 0
        rsLF[2] = new LockSnapshot(1,-1,3,-1,1); // lock 1
        rsLF[4] = new LockSnapshot(1,-1,3,-1,1); // lock 3
        rsLF[9] = new CompetitorSnapshot(1,0,1,4); // competitor 3
        pvObj.addRS(rsLF);
        
        // state FL
        vector<StateSnapshot*> rsFL = rs ;
        engaged.clear() ;
        engaged.push_back(4);
        // 35 arbitrarily picked. (Only engaged matters)
        cSnap = new ControllerSnapshot(engaged,busy,front,back,self,35,&sc);
        rsFL[0] = cSnap ; // controller
        rsFL[2] = new LockSnapshot(1,-1,4,-1,1); // lock 1
        rsFL[3] = new LockSnapshot(1,-1,4,-1,1); // lock 2
        rsFL[5] = new LockSnapshot(1,-1,4,-1,1); // lock 4
        rsFL[10] = new CompetitorSnapshot(1,1,2,4); // competitor 4
        pvObj.addRS(rsFL);
        
        
        // Specify the starting state
        GlobalState startPoint(pvObj.getMachinePtrs());

        // Specify the error states
        ErrorState locklock(&startPoint) ;
        CompetitorSnapshot veh4(2,1,2,4); // vehicle 4 in state 4, with front=1, back=2
        locklock.addCheck(&veh4, 10);
        CompetitorSnapshot veh3(2,0,1,4); // vehicle 3 in state 4, with front=0, back=1
        locklock.addCheck(&veh3, 9);
        pvObj.addError(&locklock) ;

        // Start the procedure of probabilistic verification. 
        // Specify the maximum probability depth to be explored
        pvObj.start(3);

        // When complete, deallocate all machines
        delete ctrl ;
        for( size_t i = 0 ; i < arrLock.size() ; ++i ) 
            delete arrLock[i];
        for( size_t i = 0 ; i < arrComp.size() ; ++i )
            delete arrComp[i];
        delete chan ;
        // Deallocate the SnapShots used to initialize RS
        for( size_t i = 0 ; i < rs.size() ; ++ i )
            delete rs[i];
        
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

