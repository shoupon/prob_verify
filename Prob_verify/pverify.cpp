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

void ProbVerifier::addError(StoppingState *es)
{
    if( es->getStateVec().size() != _macPtrs.size() ) {
        cerr << "The size of the ErrorState does not match the number of machines" << endl;
        return ;
    }
    
    _errors.push_back(es);
    
}

void ProbVerifier::addPrintStop(bool (*printStop)(GlobalState *, GlobalState *))
{
    _printStop = printStop ;
}

// The basic procedure
void ProbVerifier::start(int maxClass)
{
    try {
        int hit = 0;
        _totalStates = GSHash(10000) ;
#ifdef LOG
        cout << "Stopping states:" << endl ;
        for( size_t i = 0 ; i < _RS.size() ; ++i ) {
            cout << _RS[i]->toString() ;
        }
        cout << endl;
        cout << "Error states:" << endl;
        for( size_t i = 0 ; i < _errors.size() ; ++i ) {
            cout << _errors[i]->toString() ;
        }
        cout << endl;
#endif
        
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
        
        for( _curClass = 0 ; _curClass <= _maxClass ; ++_curClass ) {
            cout << "-------- Exploring GlobalStates of class[" << _curClass
            << "] --------" << endl ;
            // Check if the members in class[k] are already contained in STATET (_arrFinStart).
            // If yes, remove the member from class[k];
            // otherwise, add that member to STATET
            GSMap::iterator it = _arrClass[_curClass].begin();
            cout << "There are " << _arrClass[_curClass].size() << " GlobalStates "
                 << "in class[" << _curClass << "]" << endl;
            while( it != _arrClass[_curClass].end() ) {
                GlobalState* st = it->first ;
                if( _curClass == 0 ) {
                    if( isStopping(st) ) {
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
                // Remove st from class[k] (_arrClass[_curClass])
                _arrClass[_curClass].erase(it);

                                
                if( st->getDistance() > _max ) {
                    cout << _max << "composite states explored." << endl ;
                    cout << "Livelock found after " << st->getProb()
                         << " low probability transitions" << endl ;
                    cout << "Total GlobalStates in unique table: " << _totalStates.size()
                         << endl ;
                    
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
                    st->updateParents();
                    // Increase the threshold of livelock detection
                    _max += st->size();
                }
                st->updateTrip();
                size_t nChilds = st->size();
                
                // If the explored GlobalState st is in RS, add st to STATETABLE
                // (_arrFinRS), so when the later probabilistic search reaches st, the
                // search will stop and explore some other paths
                if( isStopping(st) ) {
                    if( find( _arrRS, st) != _arrRS.end() ) {
                        insert(_arrFinStart, st);
                        insert(_arrFinRS, st);
                        
                        cout << "Stopping state reached: " << _max << endl ;
                        //st->printOrigins(_printStop);
                    }
                    else {
                        insert(_arrRS, st) ;
                    }
                    
                    // for each child of this stopping state, add this stopping state
                    // in to the list of origin stopping states of the child (_origin)
                    // Allow one to trace back to the stopping state that leads to this
                    // state
                    for( size_t ci = 0 ; ci < st->size() ; ++ci ) {
                        st->getChild(ci)->addOrigin(st);
                    }
                }
                
#ifdef LOG
                cout << "Exploring " << st->toString() << ":" << endl ;  ;
#endif
                if( nChilds == 0 ) {
                    // No child found. Report deadlock
                    cout << _max << "composite states explored." << endl ;
                    cout << "Deadlock found." << endl ;
                    
                    vector<GlobalState*> seq;
                    st->pathRoot(seq);
                    printSeq(seq);
                    
                    return ;
                }
                else if( isError(st) ) {
                    // The child matches the criterion of an ErrorState.
                    // Print out the path that reaches this error state.
                    cout << _max << "composite states explored." << endl ;
                    cout << endl << "Error state found: " << endl ;
                    cout << st->toString() << endl;
                    
                    vector<GlobalState*> seq;
                    st->pathRoot(seq);
                    printSeq(seq);                    
                    
                    return ;
                }
                else {
                    // Add the computed childs to class array ST[class].
                    for( size_t idx = 0 ; idx < st->size() ; ++idx ) {
                        
                        GlobalState* childNode = st->getChild(idx);
                        int prob = st->getProb(idx);                        
#ifdef LOG
                        int dist = childNode->getDistance();
                        cout << childNode->toString() << " Prob = " << prob
                        << " Dist = " << dist << endl;
#endif
                        // Add the childnode to _arrClass[_curClass] provided that it is not
                        // already a member of _arrFinStart (STATETABLE as in paper)
                        if( find(_arrFinStart,childNode) == _arrFinStart.end() ) {
                            addToClass(childNode, prob);
                        }
                        else {
                            // Do something else, such as print out the probability
                            cout << "Stopping state reached: " << _max << endl ;
                            //childNode->printOrigins(_printStop);
                            if(GlobalState::removeBranch(childNode))
                                break;
                            else
                                idx--;
                        }
                    }
#ifdef LOG
                    cout << endl   ;
#endif
                }
                
                // Finish exploring st.                 
            } // while (explore the global state in class[_curClass] until all the global states in the class
            // are explored
            cout << _max << "composite states explored." << endl ;
            cout << "Total GlobalStates in unique table: " << _totalStates.size() << endl ;
        } // for (explore all the class until class[0] through class[_maxClass-1] are fully explored
        
        // Conclude success
        cout << endl;
        cout << "Procedure complete" << endl ;
        cout << _max << "composite states explored." << endl ;
        cout << "Total GlobalStates in unique table: " << _totalStates.size() << endl ;
        cout << "Up to " << maxClass << " low probability transitions considered." << endl ;
        cout << "No deadlock or livelock found." << endl;
        
    } // try
    catch ( GlobalState* st ) {
#ifdef TRACE_UNMATCHED
        vector<GlobalState*> seq;
        st->pathRoot(seq);
        printSeq(seq);
#endif
        
    } // catch

}
/*
void ProbVerifier::setRS(vector<GlobalState*> rs) 
{     
    for( size_t ii = 0 ; ii < rs.size() ; ++ii ) {
        _RS.insert( GSMapPair(rs[ii],0) );
    }
} // setRS()*/

void ProbVerifier::addRS(StoppingState* rs)
{
    _RS.push_back(rs);
}

GSVecMap::iterator ProbVerifier::find(GSVecMap& collection, GlobalState* gs)
{
    string gsStr = gs->toString();
    if( collection.empty() )
        return collection.end();

    return collection.find(gsStr);
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
#ifdef DEBUG
        if( isError(seq[ii]) ) {
            cout << endl ;
            cout << "Error state found in sequence: " << seq[ii]->toString() << endl ;
        }
#endif
    }
    cout << seq.back()->toString() << endl ;
}

bool ProbVerifier::isError(const GlobalState* obj)
{
    return findMatch(obj, _errors) ;
}

bool ProbVerifier::isStopping(const GlobalState* obj)
{
    return findMatch(obj, _RS) ;
}

bool ProbVerifier::findMatch(const GlobalState* obj,
                             const vector<StoppingState*>& container)
{
    for( size_t i = 0 ; i < container.size() ; ++i ) {
        if( container[i]->match(obj) )
            return true ;
    }
    return false;
}

void ProbVerifier::printStopping(const GlobalState *obj)
{
    cout << "Stopping state reached: "
         << obj->toString() << endl;
}

/*
void ProbVerifier::printRS()
{
    GSVecMap::iterator it ;
    for( it = _RS.begin() ; it != _RS.end() ; ++it ) {
        cout << "[" ;
        for( size_t i = 0 ; i < it->first.size()-1 ; ++i ) {
            cout << it->first.at(i) << "," ;
        }
        cout << it->first.back() << "]" ;
    }
    cout << endl ;
}*/

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
