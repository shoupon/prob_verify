#include "controller.h"
#include "lock_utils.h"
#include "competitor.h"
#include "lock.h"
#include "../define.h"

Controller::Controller( Lookup* msg, Lookup* mac, int num, int delta )
: StateMachine(msg, mac), _numVehs(num), _nbrs(num)
, _time(0), _delta(delta), _altBit(0)
{ 
    _name = "controller" ;
    _machineId = machineToInt(_name);
    
    _actives.resize(num,false);
    _actSetting.resize(num,false);
    _seqReg = new SeqCtrl(num);
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
    if( msg == "complete" || msg == "abort") {
        int veh = inMsg->getParam(0) ;
        if( typeid(*inMsg) == typeid(CompetitorMessage) ) {
            _busy[veh] = -1 ;
            removeEngaged(veh);
            _time++;
            return 3 ;
        }
        else if( typeid(*inMsg) == typeid(LockMessage) ) {
            int master = inMsg->getParam(1);
            if( _fronts[master] == veh ) {
                _fronts[master] = -1;
            }
            else if( _backs[master] == veh ) {
                _backs[master] = -1;
                
            }
            else if( _selves[master] == veh ) {
                _selves[master] = -1;
            }
            else {
                /*
                // This shouldn't happen:
                // The lock send complete message to the controller, but the controller
                // thought the lock is being released
                assert(false); */
            }
            removeEngaged(master);
            _time++;
            return 3;
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
            int f = _fronts[veh];
            int b = _backs[veh];
            
            // create response
            int vId = machineToInt(Lock_Utils::getCompetitorName(veh));
            int fId = machineToInt(Lock_Utils::getLockName(f));
            int bId = machineToInt(Lock_Utils::getLockName(b));
            int sId = machineToInt(Lock_Utils::getLockName(veh));
            int dstMsgId = messageToInt("timeout");
            // send message to the competitor or the lock that has not been reset
            // i.e. the some messages are loss during the course 
            ControllerMessage* vMsg =
                new ControllerMessage(0,vId,0,dstMsgId,_machineId,veh);
            ControllerMessage* fMsg =
                new ControllerMessage(0,fId,0,dstMsgId,_machineId,veh);
            ControllerMessage* bMsg =
                new ControllerMessage(0,bId,0,dstMsgId,_machineId,veh);
            ControllerMessage* sMsg =
                new ControllerMessage(0,sId,0,dstMsgId,_machineId,veh);
            if( _busy[veh] != -1 )
                outMsgs.push_back(vMsg);
            if( _fronts[veh] != -1 )
                outMsgs.push_back(fMsg);
            if( _backs[veh] != -1 )
                outMsgs.push_back(bMsg);
            if( _selves[veh] != -1 )
                outMsgs.push_back(sMsg);
            
            // Change state
            if( _busy[veh] >= _time )
                throw logic_error("Relative event order is not maintained");
            _time++ ;
            _busy[veh] = -1 ;
            _fronts[veh] = -1 ;
            _backs[veh] = -1;
            _selves[veh] = -1 ;
            removeEngaged(veh);
            
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
                if( _seqReg->isAllow((int)i)) {
                    --count ;
                }
#ifdef VERBOSE
                else {
                    cout << "Merge initiation denied" << endl ;
                }
#endif
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
                    _selves[i] = (int)i ;
                    _fronts[i] = f ;
                    _backs[i] = b ;
                    _engaged.push_back((int)i);
                    _seqReg->engage((int)i) ;
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
    StateSnapshot* ret = new ControllerSnapshot(_engaged, _busy,
                                                _fronts, _backs, _selves, _time, _seqReg);
    return ret ;
}

void Controller::restore(const StateSnapshot* ss)
{
    if( typeid(*ss) != typeid(ControllerSnapshot) )
        return ;

    const ControllerSnapshot* cs = dynamic_cast<const ControllerSnapshot*>(ss);
    _seqReg = new SeqCtrl( *(cs->_ss_seqCtrl) ); 
    _engaged = cs->_ss_engaged ;
    _busy = cs->_ss_busy ;
    _fronts = cs->_ss_front ;
    _backs = cs->_ss_back;
    _selves = cs->_ss_self;
    _time = cs->_ss_time ;
    
}

void Controller::reset() 
{
    _actives = _actSetting ;
    _seqReg->reset() ;
    
    _busy.clear() ;
    _fronts.clear() ;
    _backs.clear() ;
    _selves.clear();
    
    _busy.resize(_numVehs, -1);
    _fronts.resize(_numVehs, -1);
    _backs.resize(_numVehs, -1);
    _selves.resize(_numVehs, -1);
    
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

void Controller::setActives(const vector<bool>& input)
{
    if(input.size()==_actives.size()) {
        _actives=input;
        _actSetting=input;
    }
}

bool Controller::removeEngaged(int i)
{
    if( _busy[i] != -1 )
        return false ;
    if( _selves[i] != -1 )
        return false ;
    if( _fronts[i] != -1 )
        return false ;
    if( _backs[i] != -1 )
        return false ;
    
    vector<int>::iterator it ;
    for( it = _engaged.begin() ; it != _engaged.end() ; ++it ) {
        if( *it == i ) {
            _engaged.erase(it);
            _seqReg->disengage(i);
            return true ;
        }
    }
    
    //assert(false);
    return false;
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

ControllerSnapshot::ControllerSnapshot(vector<int> eng, vector<int> busy,
                                       vector<int> front, vector<int> back,
                                       vector<int> self,
                                       int time, const SeqCtrl* seqCtrl)
:_ss_engaged(eng), _ss_busy(busy), _ss_time(time),
 _ss_back(back), _ss_front(front), _ss_self(self)
{
    int sumEng = 0 ;
    for( size_t i = 0 ; i < _ss_engaged.size() ; ++i ) {
        sumEng += _ss_engaged[i];
        sumEng = sumEng << 4;
    }
    _hash_use  = (sumEng << 4) + (int)_ss_engaged.size() ;
    
    _ss_seqCtrl = new SeqCtrl(*seqCtrl) ;
}

ControllerSnapshot::ControllerSnapshot(const ControllerSnapshot& item)
:_ss_engaged(item._ss_engaged), _ss_busy(item._ss_busy), _ss_front(item._ss_front),
 _ss_back(item._ss_back), _ss_self(item._ss_self), _ss_time(item._ss_time)
{
    _ss_seqCtrl = new SeqCtrl(*(item._ss_seqCtrl)) ;
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
#ifdef CONTROLLER_TIME
    ss << _ss_time << "(" ;
#else
    ss << "(" ;
#endif
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
    return new ControllerSnapshot(*this) ;
    
}
