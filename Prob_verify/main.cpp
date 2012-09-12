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
        vector<bool> active(5,false) ;
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

        // Specify the global states in the set RS
        // TODO: only all zero RS
        // create snapshots
        vector<StateSnapshot*> rs;
        // controller snapshot
        vector<int> engaged ;
        vector<int> busy(num,-1) ;
        StateSnapshot* cSnap = new ControllerSnapshot(engaged,busy,busy,busy,busy,0);
        rs.push_back(cSnap);
        // lock snapshots
        for( int i = 0 ; i < num; ++i ) {
            // LockSnapshot(int ts, int t2, int oldCom, int newCom, int state)
            StateSnapshot* lSnap = new LockSnapshot(0,0,-1,-1,0);
            rs.push_back(lSnap);
        }
        // competitor snapshots
        for( int i = 0 ; i < num ; ++i ) {
            //CompetitorSnapshot(int t, int front, int back,int state)
            StateSnapshot* compSnap = new CompetitorSnapshot(0,-1,-1,0);
            rs.push_back(compSnap);
        }
        // channel snapshots
        StateSnapshot* chanSnap = new ChannelSnapshot();
        rs.push_back(chanSnap);
                                                              
        pvObj.addRS(rs);

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

