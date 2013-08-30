#include <cassert>

#include "pverify.h"

//#define VERBOSE

/**
 Add the successor GlobalState* childNode to appropriate probability class, provided
 that childNode does not already exist in the class of GlobalStates; Also this function
 maintains a table of reachable states
 */
bool ProbVerifier::addToClass(GlobalState* childNode, int prob)
{
    if( prob >= _maxClass )
        return false;
    //if( find(_arrFinRS,childNode) != _arrFinRS.end() )
      //  return false ;
    
    GSMapIter it = _arrClass[prob].find(childNode)  ;
    if( it != _arrClass[prob].end() ) {
        it->first->merge(childNode) ;
        return false ;
    }
    else {
        _arrClass[prob].insert( GSMapPair(childNode, 0) );
        addToReachable(childNode) ;
        return true ;
    }
}

bool ProbVerifier::addToReachable(GlobalState *gs)
{
    string str = gs->toString() ;
    if( _reachable.find(str) == _reachable.end() ) {
        _reachable.insert(str) ;
        return true ;
    }
    else
        return false;
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
#ifdef LOG
        cout << "Stopping states:" << endl ;
        for( size_t i = 0 ; i < _stops.size() ; ++i ) {
            cout << _stops[i]->toString() ;
        }
        cout << endl;
        cout << "Error states:" << endl;
        for( size_t i = 0 ; i < _errors.size() ; ++i ) {
            cout << _errors[i]->toString() ;
        }
        cout << endl;
#endif
        
        _maxClass = maxClass ;
        _arrClass.resize(maxClass+1, GSMap());
        for( size_t ii = 0 ; ii < _macPtrs.size() ; ++ii )
            _macPtrs[ii]->reset();
        
        if( _checker != 0 ) {
            _root = new GlobalState(_macPtrs, _checker->initState() ) ;
        }
        else {
            _root = new GlobalState(_macPtrs) ;
        }
        
        // Initialize _arrClass[0] to contain the initial global state.
        // the other maps are initialized null.
        _arrClass[0].insert( GSMapPair(_root,0) );
        GlobalState::init(_root);
        addToReachable(_root);
        
        for( _curClass = 0 ; _curClass <= _maxClass ; ++_curClass ) {
            cout << "-------- Exploring GlobalStates of class[" << _curClass
                 << "] --------" << endl ;
            GSMap::iterator it = _arrClass[_curClass].begin();
            cout << "There are " << _arrClass[_curClass].size() << " GlobalStates "
                 << "in class[" << _curClass << "]" << endl;
            // Check if the members in class[k] are already contained
            // in STATET (_arrFinStart).
            // If yes, remove the member from class[k];
            // otherwise, add that member to STATET
            while( it != _arrClass[_curClass].end() ) {
                GlobalState* st = it->first ;
                if( _curClass == 0 ) {
                    insert(_arrFinStart, st);
                }
                else {
                    if( find(_arrFinStart,st) != _arrFinStart.end() ) {
                        // st is already contained in STATET (_arrFinStart)
                        // which means st is already explored
                        // TODO: should merge globalstate instead
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
                GSMap::iterator it = _arrClass[_curClass].begin();
                GlobalState* st = it->first ;
                _arrClass[_curClass].erase(it);
                
                if( checkLivelock(st) )
                    return ;
                if( checkError(st) )
                    return ;
                                
                if( !st->hasChild() ) {
                    // Compute all the globalstate's childs
                    st->findSucc();
#ifdef TRACE
                    st->updateParents();
#endif
                    // Increase the number of step taken
                    _max += st->size();
                }
                
                if( checkDeadlock(st) )
                    return;
                
                st->updateTrip();
                
                // If the explored GlobalState st is in RS, add st to STATETABLE
                // (_arrFinRS), so when the later probabilistic search reaches st, the
                // search will stop and explore some other paths
                if( isStopping(st) ) {
                    cout << "Stopping state reached: " << _max << endl ;
#ifdef TRACE_STOPPING
                    vector<GlobalState*> seq;
                    if( st != _root ) {
                        st->pathRoot(seq);
                        printSeq(seq);
                    }
#endif
                    insert(_arrFinRS, st);
                    if( find( _arrRS, st) == _arrRS.end() ) {
                        insert(_arrRS, st);
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
                printStep(st) ;
#endif
                // Processes the computed successors
                // Add the childnode to _arrClass[_curClass] provided that it is not
                // already a member of _arrFinStart (STATETABLE as in paper)
                for( size_t idx = 0 ; idx < st->size() ; ++idx ) {
                    GlobalState* childNode = st->getChild(idx);                    
                    if( isEnding(childNode) ) {
                        cout << "Ending state reached. " << endl;
                        printStat() ;
#ifdef TRACE
                        idx--;
                        if(GlobalState::removeBranch(childNode)) 
                            break;
#else
                        delete childNode;
#endif
                    }
                    else if( find(_arrFinRS, childNode) != _arrFinRS.end() ) {
                        cout << "Explored stopping state reached." << endl ;
                        printStat() ;
#ifdef TRACE
                        idx-- ;
                        if(GlobalState::removeBranch(childNode))
                            break;
#else
                        delete childNode;
#endif
                    }
                    else{
                        addToClass(childNode, childNode->getProb());
                    }
                }

                // Finish exploring st.
#ifndef TRACE
                //delete st;
#endif
                
            } // while (explore the global state in class[_curClass] until all the global
              // states in the class are explored
            printStat();
        } // for (explore all the class until class[0] through class[_maxClass-1]
          // are fully explored
        
        // Conclude success, print statistics
        cout << "Procedure complete." << endl;
        printStat();
        printFinRS();
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

void ProbVerifier::addSTOP(StoppingState* rs)
{
    _stops.push_back(rs);
}

void ProbVerifier::addEND(StoppingState *end)
{
    _ends.push_back(end);
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
    if( seq.size() == 0 )
        return;
    for( int ii = 0 ; ii < (int)seq.size()-1 ; ++ii ) {
        if( seq[ii]->getProb() != seq[ii+1]->getProb() ) {
            int d = seq[ii+1]->getProb() - seq[ii]->getProb() ;
            if( d == 1 )
                cout << seq[ii]->toString() << " -p-> ";
            else
                cout << seq[ii]->toString() << " -p" << d << "->";
        }
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

bool ProbVerifier::isError(GlobalState* obj)
{
    bool result ;
    if( _checker != 0 ) {
        result = _checker->check(obj->chkState(), obj->getStateVec()) ;
    }
    return ( !result || findMatch(obj, _errors) );
}

bool ProbVerifier::isStopping(const GlobalState* obj)
{
    return findMatch(obj, _stops) ;
}

bool ProbVerifier::isEnding(const GlobalState *obj)
{
    return findMatch(obj, _ends);
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

void ProbVerifier::printStep(GlobalState *obj)
{
    cout << "Exploring " << obj->toString() << ":" << endl ;  ;
    for( size_t idx = 0 ; idx < obj->size() ; ++idx ) {
        GlobalState* childNode = obj->getChild(idx);
        int prob = childNode->getProb();
        int dist = childNode->getDistance();
        cout << childNode->toString()
             << " Prob = " << prob << " Dist = " << dist << endl;
    }
}

void ProbVerifier::printStat()
{
    cout << _reachable.size() << " reachable states found." << endl ;
    cout << _max << " transitions taken." << endl ;
    cout << _arrFinStart.size() << " entry points discoverd (_arrFinStart)" << endl;
    cout << _arrFinRS.size() << " stopping states discovered (_arrFinRS)" << endl ;
}

void ProbVerifier::printFinRS()
{
    cout << "StoppingStates encountered:" << endl ;
    for( GSVecMap::iterator it = _arrFinRS.begin(); it != _arrFinRS.end() ; ++it ) {
        cout << it->first << endl ;
    }
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

bool ProbVerifier::checkDeadlock(GlobalState *gs)
{
    if( gs->size() == 0 ) {
        // No child found. Report deadlock
        cout << "Deadlock found." << endl ;
        printStat() ;
#ifdef TRACE
        vector<GlobalState*> seq;
        gs->pathRoot(seq);
        printSeq(seq);
#endif
        return true;
    }
    else
        return false;
}

bool ProbVerifier::checkLivelock(GlobalState* gs)
{
    // Theorem 1 in [Maxemchuk and Sabnani]
    if( gs->getDistance() > _reachable.size() - _arrFinRS.size() ) {
        cout << "Livelock found after " << gs->getProb()
             << " low probability transitions" << endl ;
        printStat();
#ifdef TRACE
        vector<GlobalState*> seq;
        gs->pathRoot(seq);
        printSeq(seq);
#endif
        return true;
    }
    else
        return false;
}

bool ProbVerifier::checkError(GlobalState *gs)
{
    if( isError(gs) ) {
        // The child matches the criterion of an ErrorState.
        // Print out the path that reaches this error state.
        cout << "Error state found: " << gs->toString() << endl;
        printStat() ;
#ifdef TRACE
        vector<GlobalState*> seq;
        gs->pathRoot(seq);
        printSeq(seq);
#endif
        return true;
    }
    else
        return false;
}
