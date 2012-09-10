#include "controller.h"
#include "lock_utils.h"

Controller::Controller( Lookup* msg, Lookup* mac, int num, int delta )
: StateMachine(msg, mac), _numVehs(num), _nbrs(num)
, _time(0), _delta(delta), _altBit(0)
{ 
    _name = "controller" ;
    _machineId = machineToInt(_name);

    reset();
}

int Controller::transit(MessageTuple* inMsg, vector<MessageTuple*>& outMsgs,
                bool& high_prob, int startIdx ) 
{
    outMsgs.clear();
    high_prob = true ;

    if( startIdx != 0 )
        return -1;

    string msg = IntToMessage( inMsg->destMsgId() ) ;
    if( msg == "complete" ) {
        int veh = inMsg->getParam(0) ;
        // Remove that vehicle id from _engaged list
        for( vector<int>::iterator it = _engaged.begin() ; it != _engaged.end() ; ++it) {
            if( *it == veh ) {
                _engaged.erase(it) ;
                assert(_time > _busy[veh]) ;
                _busy[veh] = -1 ;
                _time++;
                return 3 ;
                break ;
            }
        }
    }
    else if( msg == "abort" ) {
        int veh = inMsg->getParam(0);
        // Remove that vehicle id from _engaged list
        for( vector<int>::iterator it = _engaged.begin() ; it != _engaged.end() ; ++it) {
            if( *it == veh ) {
                _engaged.erase(it) ;
                assert(_time > _busy[veh]) ;
                _busy[veh] = -1 ;
                _time++;
                return 3 ;
                break ;
            }
        }
    }
    
    return -1;
}

int Controller::nullInputTrans(vector<MessageTuple*>& outMsgs,
                               bool& high_prob, int startIdx ) 
{ 
    outMsgs.clear();
    high_prob = true ;
    // Used to skip the timeout events
    bool skip = false ;

    if( startIdx == 0 ) {
        if( !_engaged.empty() ) {
            // timeout event
            int veh = _engaged.front() ;            
            int f = _nbrs[veh].front().first ;
            int b = _nbrs[veh].front().second ;
            // create response
            int vId = machineToInt(Lock_Utils::getCompetitorName(veh));
            int fId = machineToInt(Lock_Utils::getCompetitorName(f));
            int bId = machineToInt(Lock_Utils::getCompetitorName(b));
            int dstMsgId = messageToInt("timeout");            
            ControllerMessage* vMsg = new ControllerMessage(0,vId,0,dstMsgId,_machineId);
            ControllerMessage* fMsg = new ControllerMessage(0,fId,0,dstMsgId,_machineId);
            ControllerMessage* bMsg = new ControllerMessage(0,bId,0,dstMsgId,_machineId);
            outMsgs.push_back(vMsg);
            outMsgs.push_back(fMsg);
            outMsgs.push_back(bMsg);
            // Change state
            if( _busy[veh] >= _time )
                throw logic_error("Relative event order is not maintained");
            _time++ ;
            _busy[veh] = -1 ; 
            _engaged.erase(_engaged.begin());
            
            high_prob = false ;

            return 1;
        }
        else {
            skip = true ;
        }

    }

    if( startIdx < 0 )
        return -1;
    else if( startIdx > 0 || skip ) {
        // check for initiation
        int count ;
        if( skip )
            count = 1; // find the first merge start event
        else
            count = startIdx ;

        size_t i = 0 ;
        while( i < _busy.size() ) {
            if( _busy[i] == -1 && _actives[i] == true ) {
                --count ;
                if( count == 0 ) {
                    // merge start event
                    int f = _nbrs[i].front().first ;
                    int b = _nbrs[i].front().second ;
                    // create response
                    int dstId = machineToInt(Lock_Utils::getCompetitorName((int)i));
                    int dstMsgId = messageToInt("init");            
                    ControllerMessage* initMsg = new ControllerMessage(0,dstId,0,dstMsgId,
                                                                       _machineId);
                    initMsg->addParams(_time, f, b);
                    outMsgs.push_back(initMsg);                    
                    // Change state
                    _busy[i] = _time ;
                    _engaged.push_back((int)i);
                    _time++;                    

                    if( skip )
                        return 2 ; // next time start by finding the second merge event
                    else 
                        return startIdx + 1;
                }               
            }
            ++i ;
        }
    }

    return -1;
}

StateSnapshot* Controller::curState() 
{
    StateSnapshot* ret = new ControllerSnapshot(_engaged, _busy, _time);
    return ret ;
}

void Controller::restore(const StateSnapshot* ss)
{
    if( typeid(*ss) != typeid(ControllerSnapshot) )
        return ;

    const ControllerSnapshot* cs = dynamic_cast<const ControllerSnapshot*>(ss);
    _engaged = cs->_ss_engaged ;
    _busy = cs->_ss_busy ;
    _time = cs->_ss_time ;
}

void Controller::reset() 
{
    _actives.resize(_numVehs, true);  
    _busy.resize(_numVehs, -1);
    _time = 0 ;
    _altBit = 0 ;
    _engaged.clear() ;
}

void Controller::allNbrs()
{
    for( size_t i = 0 ; i < _numVehs ; ++i ) 
        _nbrs[i].clear();

    for( int i = 0 ; i < _numVehs ; ++i ) {
        for( int j = 0 ; j < _numVehs ; ++j ) {
            if( j == i )
                continue ;
            for( int  k = j+1 ; k < _numVehs ; ++k ) {
                if( k == i ) 
                    continue ;
                _nbrs[i].push_back( Neighbor(i,j) ) ;
                _nbrs[i].push_back( Neighbor(j,i) ) ;
            }
        }
    }
}
 
int ControllerMessage::getParam(size_t arg) 
{
    switch(arg) {
        case 0:
            return _timestamp;
            break ;
        case 1:
            return _front;
            break;
        case 2:
            return _back;
            break ;
        default:
            return -1;
            break;
    }
}

void ControllerMessage::addParams(int time, int front, int back)
{
    _timestamp = time;
    _front = front;
    _back = back;
    _nParams = 3;
}

ControllerSnapshot::ControllerSnapshot(vector<int> eng, vector<int> busy, int time)
:_ss_engaged(eng), _ss_busy(busy), _ss_time(time)
{
    int sumEng = 0 ;
    for( size_t i = 0 ; i < _ss_engaged.size() ; ++i ) {
        sumEng += _ss_engaged[i];
    }
    sumEng +=  _ss_time ;
    _hash_use  = (sumEng << 2) + (int)_ss_engaged.size() ;
}

int ControllerSnapshot::curStateId() const
{
    if( _ss_engaged.empty() )
        return 0 ;
    else
        return 1;
}
// Returns the name of current state as specified in the input file
string ControllerSnapshot::toString()
{
    stringstream ss;
    ss << _ss_time << "(" ;
    if( _ss_engaged.empty() )
        ss << ")" ;
    else {
        for( size_t i = 0 ; i < _ss_engaged.size() - 1 ; ++i ) {
            ss << _ss_engaged[i] << "," ;
        }
        ss << _ss_engaged.back() << ")" ;
    }
    return ss.str();
}

ControllerSnapshot* ControllerSnapshot::clone() const 
{
    return new ControllerSnapshot(_ss_engaged, _ss_busy, _ss_time) ;
    
}
