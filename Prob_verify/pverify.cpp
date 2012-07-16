#include <cassert>

#include "pverify.h"
    
#define VERBOSE


bool ProbVerifier::addToClass(GlobalState* childNode, int toClass)
{
    if( toClass >= _maxClass ) 
        return false;

    /*GSMapIter it = _computedClass[toClass].find(childNode);
    if( it != _computedClass[toClass].end() ) {
        it->first->increaseVisit(childNode->getVisit());
        return false;
    }
    else {
        it = _arrClass[toClass].find(childNode)  ;
        if( it != _arrClass[toClass].end() ) {
            it->first->increaseVisit(childNode->getVisit());
            return false ;
        }
        else {
            _arrClass[toClass].insert( GSMapPair(childNode, 0) );
            return true ;
        }
    }*/


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



// The basic procedure
void ProbVerifier::start(int maxClass)
{
    _maxClass = maxClass ;
    _arrClass.resize(maxClass, GSMap());
    _computedClass.resize(maxClass, GSMap());
    for( size_t ii = 0 ; ii < _macPtrs.size() ; ++ii ) 
        _macPtrs[ii]->reset();

    int nMacs = _macPtrs.size() ;
    _root = new GlobalState(_macPtrs) ; 
    GlobalState::init(_root);

    // Initialize _arrClass[0] to contain the initial global state. 
    // the other maps are initialized null.    
    _arrClass.push_back( GSMap() );   
    _arrClass[0].insert( GSMapPair(_root,0) );
#ifdef VERBOSE
    cout << _root->toString() << endl ;
#endif
   
    for( _curClass = 0 ; _curClass < _maxClass ; ++_curClass ) {
        // Check if the members in class[k] are already contained in STATET (_arrFinStart).
        // If yes, remove the member from class[k];
        // otherwise, add that member to STATET
        GSMap::iterator it = _arrClass[_curClass].begin();
        while( it != _arrClass[_curClass].end() ) {
            GlobalState* st = it->first ;
            if( _curClass == 0 ) { 
                if( _RS.find(  st->getStateVec() ) != _RS.end() ) {
                    // If *ptr is a member of RS, add it to STATETABLE (_arrFinRS) 
                    // and to STATET (_arrFinStart)
                    _arrFinRS.insert(GSMapPair(st,_curClass)) ;
                    _arrFinStart.insert(GSMapPair(st,_curClass)) ;
                }
            }
            else {
                if( _arrFinStart.find(st) != _arrFinStart.end() ) {
                    // st is already contained in STATET (_arrFinStart)
                    // which means st is already explored
                    _arrClass[_curClass].erase(it++);
                    continue ;
                }
                else {
                    _arrFinStart.insert(GSMapPair(st,_curClass)) ;
                }
            }
            it++;
        } // while

        while( !_arrClass[_curClass].empty() ) {
            // Pop a globalstate pointer ptr from class[k] (_arrClass[_curClass])
            GSMapConstIter it = _arrClass[_curClass].begin();            
            GlobalState* st = it->first ;           

            if( !st->hasChild() ) {
                // Compute all the globalstate's childs
                st->findSucc();
#ifdef VERBOSE
                cout << st->toString() << ": "  ;
#endif
                size_t nChilds = st->size();
                // Increase the threshold of livelock detection
                _max += nChilds;
                if( nChilds == 0 ) {
                    // No child found. Report deadlock TODO (Printint the sequence, blah blah)
                    cout << "Deadlock found." << endl ;
                    return ;
                }
                else {                
                    // Explore all its childs
                    for( size_t idx = 0 ; idx < nChilds ; ++idx ) {
                        GlobalState* childNode = st->getChild(idx);
                        if( childNode->getDistance() > _max ) {
                            cout << "Livelock found. " << endl ;
                            return ;
                        }                    

                        int prob = st->getProb(idx);                    
#ifdef VERBOSE
                        cout << childNode->toString() << " Prob = " << prob << ", "  ;
#endif
                        if( _arrFinRS.find(childNode) == _arrFinRS.end() ) {
                            // If the child node is not already a member of STATETABLE
                            addToClass(childNode, prob);
                        }
                    }   
#ifdef VERBOSE 
                    cout << endl   ;
#endif
                }
            }

            // Finish exploring st. Remove st from class[k] (_arrClass[_curClass])
            //_computedClass[_curClass].insert(*it);
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

void ProbVerifier::addRS(vector<int> rs)
{
    _RS.insert( GSVecMapPair(rs,0) );
    // This should be also added to GlobalState::_uniqueTable

}
