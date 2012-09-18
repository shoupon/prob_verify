#include <cassert>

#include "pverify.h"
    
//#define VERBOSE


bool ProbVerifier::addToClass(GlobalState* childNode, int toClass)
{
    if( toClass >= _maxClass ) 
        return false;
    
    // If the child node is already a member of STATETABLE, which indicates the protocol under test
    // has successfully completed its function, there is no need to explore this child node later on, 
    // so do not add this child node to ST[k] (_arrClass[toClass])
    if( find(_arrFinRS,childNode) != _arrFinRS.end() )
        return false ;

    GSMapIter it = _arrClass[toClass].find(childNode)  ;
    if( it != _arrClass[toClass].end() ) {
        it->first->increaseVisit(childNode->getVisit());
        return false ;
    }
    else {
        _arrClass[toClass].insert( GSMapPair(childNode, 0) );
        return true ;
    }    

}

void ProbVerifier::addError(ErrorState *es)
{
    if( es->getStateVec().size() != _macPtrs.size() ) {
        cerr << "The size of the ErrorState does not match the number of machines" << endl;
        return ;
    }
    
    _errors.push_back(es);
    
}

// The basic procedure
void ProbVerifier::start(int maxClass)
{
    _maxClass = maxClass ;
    _arrClass.resize(maxClass, GSMap());
    for( size_t ii = 0 ; ii < _macPtrs.size() ; ++ii ) 
        _macPtrs[ii]->reset();

    _root = new GlobalState(_macPtrs) ; 
    GlobalState::init(_root);
    _root->setRoot();

    // Initialize _arrClass[0] to contain the initial global state. 
    // the other maps are initialized null.    
    _arrClass.push_back( GSMap() );   
    _arrClass[0].insert( GSMapPair(_root,0) );
#ifdef VERBOSE
    cout << _root->toString() << endl ;
#endif
   
    for( _curClass = 0 ; _curClass < _maxClass ; ++_curClass ) {
        cout << "-------- Exploring GlobalStates of class[" << _curClass
             << "] --------" << endl ;
        // Check if the members in class[k] are already contained in STATET (_arrFinStart).
        // If yes, remove the member from class[k];
        // otherwise, add that member to STATET
        GSMap::iterator it = _arrClass[_curClass].begin();
        while( it != _arrClass[_curClass].end() ) {
            GlobalState* st = it->first ;
            if( _curClass == 0 ) { 
                if( find(_RS, st ) != _RS.end() ) {
                    // If *ptr is a member of RS, add it to STATETABLE (_arrFinRS) 
                    // and to STATET (_arrFinStart)
                    insert(_arrFinRS, st );
                    insert(_arrFinStart, st);                    
                }
            }
            else {
                if( find(_arrFinStart,st) != _arrFinStart.end() ) {
                    // st is already contained in STATET (_arrFinStart)
                    // which means st is already explored
                    _arrClass[_curClass].erase(it++);
                    continue ;
                }
                else {
                    insert(_arrFinStart, st);                    
                }
            }
            it++;
        } // while

        while( !_arrClass[_curClass].empty() ) {
            // Pop a globalstate pointer ptr from class[k] (_arrClass[_curClass])
            GSMapConstIter it = _arrClass[_curClass].begin();            
            GlobalState* st = it->first ;              

            if( st->getDistance() > _max ) {
                cout << "Livelock found. " << endl ;

                vector<GlobalState*> seq;
                st->pathCycle(seq);
                printSeq(seq);
                
                return ;
            }   


            if( !st->hasChild() ) {
                // Compute all the globalstate's childs
#ifdef VERBOSE
                cout << "====  Finding successors of " << st->toString() << endl;
#endif 
                st->findSucc();                
                // Increase the threshold of livelock detection
                _max += st->size();
            }
            st->updateTrip();  
            size_t nChilds = st->size();

            // If the explored GlobalState st is in RS, add st to STATETABLE (_arrFinRS), so 
            // when the later probabilistic search reaches st, the search will stop and
            // explore
            // some other paths
            if( find(_RS,st) != _RS.end() ) {
                insert(_arrFinStart, st);   
                insert(_arrFinRS, st);
            }
      
#ifdef LOG
            cout << st->toString() << ": "  ;
#endif
            if( nChilds == 0 ) {
                // No child found. Report deadlock 
                cout << "Deadlock found." << endl ;

                vector<GlobalState*> seq;
                st->pathRoot(seq);
                printSeq(seq);

                return ;
            }
            else {                   
                // Add the computed childs to class array ST[class].
                for( size_t idx = 0 ; idx < nChilds ; ++idx ) {

                    GlobalState* childNode = st->getChild(idx);
                    
                    ErrorState* es = isError(childNode);
                    if( es != 0 ) {
                        // The child matches the criterion of an ErrorState.
                        // Print out the path that reaches this error state.
                        cout << "Error state: " << es->toString() << "reached." << endl ;
                        
                        vector<GlobalState*> seq;
                        childNode->pathRoot(seq);
                        printSeq(seq);
                        
                        return ;
                    }

                    int prob = st->getProb(idx);                   
                    int dist = childNode->getDistance();
#ifdef LOG
                    cout << childNode->toString() << " Prob = " << prob 
                                                  << " Dist = " << dist << endl;
#endif                                            
                    addToClass(childNode, prob);
                    
                }   
#ifdef LOG
                cout << endl   ;
#endif
            }            

            // Finish exploring st. Remove st from class[k] (_arrClass[_curClass])
            _arrClass[_curClass].erase(it);            
            
        } // while (explore the global state in class[_curClass] until all the global states in the class
          // are explored       

    } // for (explore all the class until class[0] through class[_maxClass-1] are fully explored
    
    // Conclude success
    cout << "No deadlock or livelock found." << endl;
}
/*
void ProbVerifier::setRS(vector<GlobalState*> rs) 
{     
    for( size_t ii = 0 ; ii < rs.size() ; ++ii ) {
        _RS.insert( GSMapPair(rs[ii],0) );
    }
} // setRS()*/

void ProbVerifier::addRS(vector<StateSnapshot*> rs)
{
    if( rs.size() != _macPtrs.size()) {
        throw runtime_error("The number of machines in the specification of addRS() function does not match the number in ProbVerifier object");
    }
    vector<string> vec(_macPtrs.size());
    for( size_t m = 0 ; m < this->_macPtrs.size() ; ++m ) {
        _macPtrs[m]->restore(rs[m]);
        StateSnapshot* snap = _macPtrs[m]->curState();
        vec[m] = snap->toString();
    }
    _RS.insert( GSVecMapPair(vec,0) );
    // This should be also added to GlobalState::_uniqueTable

}

GSVecMap::iterator ProbVerifier::find(GSVecMap& collection, GlobalState* gs)
{
    vector<string> vec = gs->getStringVec();
    if( collection.empty() )
        return collection.end();
    if( collection.begin()->first.size() != vec.size() )
        return collection.end();

    return collection.find(vec);
}

GSMap::iterator ProbVerifier::find(GSMap& collection, GlobalState* gs)
{
    return collection.find(gs);
}

void ProbVerifier::printSeq(const vector<GlobalState*>& seq) 
{
    for( int ii = 0 ; ii < (int)seq.size()-1 ; ++ii ) {
        if( seq[ii]->getProb() != seq[ii+1]->getProb() )
            cout << seq[ii]->toString() << " -p-> ";
        else
            cout << seq[ii]->toString() << " -> ";
    }
    cout << seq.back()->toString() << endl ;
}

ErrorState* ProbVerifier::isError(const GlobalState* obj)
{
    for( size_t i = 0 ; i < _errors.size() ; ++i ) {
        if( _errors[i]->match(obj) )
            return _errors[i];
    }
    return 0 ;
}

void ProbVerifier::clear()
{
    GSMap::iterator it ;
    for( size_t c = 0 ; c < _maxClass ; ++c ) {
        it = _arrClass[c].begin() ;
        while( it != _arrClass[c].end() ) {
            delete it->first ;
            ++it;
        }
    }
}

vector<StateMachine*> ProbVerifier::getMachinePtrs() const
{
    vector<StateMachine*> ret(_macPtrs.size());
    for( size_t i = 0 ; i < _macPtrs.size() ; ++i ) {
        ret[i] = _macPtrs[i] ;
    }
    return ret;
}
