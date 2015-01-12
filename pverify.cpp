#include <cassert>
#include <memory>
using namespace std;

#include "pverify.h"

//#define VERBOSE
#define LESS_VERBOSE
#define LIST_STOPPINGS
#define PROGRESS_CHUNK 1000

/**
 Add the successor GlobalState* childNode to appropriate probability class, provided
 that childNode does not already exist in the class of GlobalStates; Also this function
 maintains a table of reachable states
 */

const ProtocolError ProtocolError::kDeadLock("deadlock");
const ProtocolError ProtocolError::kLivelock("livelock");
const ProtocolError ProtocolError::kErrorState("error_state");
const ProtocolError ProtocolError::kCheckerError("checker_error");

void ProbVerifier::addError(StoppingState *es) {
  if( es->getStateVec().size() != _macPtrs.size() ) {
    cerr << "The size of the ErrorState does not match the number of machines"
         << endl;
    return ;
  }
  _errors.push_back(es);
}

void ProbVerifier::addPrintStop(bool (*printStop)(GlobalState *, GlobalState *))
{
    _printStop = printStop ;
}

void ProbVerifier::initialize() {
  start_point_ = unique_ptr<GlobalState>(new GlobalState(getMachinePtrs()));
  default_stopping_ = unique_ptr<StoppingState>(
    new StoppingState(start_point_.get()));
  for (auto ptr : getMachinePtrs())
    default_stopping_->addAllow(ptr->curState(), ptr->macId() - 1);
  addSTOP(default_stopping_.get());

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

  classes_.clear();
  entries_.clear();
  entries_.resize(1);

  for (auto ptr : _macPtrs)
    ptr->reset();

  if (_checker)
    _root = new GlobalState(_macPtrs, _checker->initState());
  else
    _root = new GlobalState(_macPtrs);

  GlobalState* first = copyToEntry(_root, 0);
  assert(isStopping(first));
  //entries_[0][_root->toString()] = _root;
}

void ProbVerifier::start(int max_class) {
  start(max_class, 1);
}

void ProbVerifier::start(int max_class, int verbose) {
  verbosity_ = verbose;
  initialize();
  classes_.resize(max_class + 1, GSClass());
  entries_.resize(max_class + 10, GSClass());
  explored_entries_.resize(max_class + 1, GSClass());
  try {
    leads_to_.clear();
    for (int cur_class = 0; cur_class <= max_class; ++cur_class) {
      if (verbosity_) {
        cout << "-------- Start exploring states in class[" << cur_class
             << "] --------" << endl ;
        printStat(cur_class);
      }
      while (entries_[cur_class].size()) {
        auto it = entries_[cur_class].begin();
        GlobalState* s = it->second;
        entries_[cur_class].erase(it);
        if (isMemberOfClasses(s)) {
          delete s;
        } else {
          copyToClass(s, cur_class);
          DFSVisit(s, cur_class);
          copyToExploredEntry(s, cur_class);
        }
      }
      if (verbosity_) {
        cout << "-------- Complete exploring states in class[" << cur_class
             << "] --------" << endl;
        printStat(cur_class);
      }
    }
    if (verbosity_) {
      cout << "Model checking procedure completes. Error not found." << endl;
      printStat();
    }
    if (verbosity_ >= 2) {
      printStoppings();
      printEndings();
    }
  } catch (GlobalState* st) {
    // TODO(shoupon): print seq even if unexpected reception is allowed;
    // TODO(shoupon): find another way to continue the procedure when unexpected
    // reception GlobalState is thrown and print the sequence that leads to this
    // global state
#ifdef TRACE_UNMATCHED
    vector<GlobalState*> seq;
    //st->pathRoot(seq);
    //printSeq(seq);
#endif
  } catch (ProtocolError pe) {
    cout << pe.toString() << endl;
  }
}

int ProbVerifier::computeBound(int target_class) {
  // start from each entry state of class[0] (stopping state/progressive state)
  // and recursively compute the constant alpha from the furthest edge state in
  // equivalent class class[target_class - 1]
  int max_alpha = 0;
  alphas_.clear();
  for (auto pair : explored_entries_[0]) {
    int alpha = DFSComputeBound(pair.first, target_class);
    if (alpha > max_alpha)
      max_alpha = alpha;
  }
  return max_alpha;
}
          
void ProbVerifier::DFSVisit(GlobalState* gs, int k) {
  stackPush(gs);
  if (verbosity_ >= 5) {
    for (int i = 0; i < dfs_stack_state_.size(); ++i)
      cout << "  ";
    cout << dfs_stack_state_.size();
    cout << "-> " << gs->toString()
         << " Prob = " << gs->getProb()
         << " Dist = " << gs->getDistance() << endl;
  }
  if (classes_[k].size() % PROGRESS_CHUNK == 0)
    cerr << "Finished exploring " << classes_[k].size()
         << " states in class[" << k << "]" << endl;
  if (isError(gs)) {
    reportError(gs);
  }
  vector<GlobalState*> childs;
  gs->findSucc(childs);
  if (!childs.size()) {
    reportDeadlock(gs);
  }
  num_transitions_ += childs.size();

  for (auto child_ptr : childs) {
    int p = child_ptr->getProb() - gs->getProb();
    if (!p && isMemberOfStack(child_ptr)) {
      // found cycle
      if (!hasProgress(child_ptr))
        reportLivelock(child_ptr);
    }
    if (!isMemberOfClasses(child_ptr)) {
      if (!p) {
        if (isEnding(child_ptr)) {
          copyToClass(child_ptr, k);
          if (verbosity_ >= 6)
            cout << "Ending state reached. " << endl;
        } else if (!k && isStopping(child_ptr)) {
          // discover new stopping state/entry point in probability class[0]
          copyToEntry(child_ptr, k);
        } else {
          DFSVisit(child_ptr, k);
          copyToClass(child_ptr, k);
          addChild(gs, child_ptr);
        }
      } else {
        child_ptr->setTrail(dfs_stack_state_);
        copyToEntry(child_ptr, k + p);
        addChild(gs, child_ptr);
      }
    }
  }
  stackPop();
}

int ProbVerifier::DFSComputeBound(const string& s, int limit) {
  int alpha = 0;
  int parent_prob = isMemberOfClasses(s)->getProb();
  for (const auto& child_str : leads_to_[s]) {
    auto child = isMemberOfClasses(child_str);
    if (child) {
      int child_prob = child->getProb();
      if (child_prob >= limit) {
        ++alpha;
      } else if (alphas_.find(child_str) == alphas_.end()) {
        DFSComputeBound(child_str, limit);
        assert(alphas_.find(child_str) != alphas_.end());
        if (parent_prob == child_prob) {
          if (alphas_[child_str] > alpha)
            alpha = alphas_[child_str];
        } else if (parent_prob < child_prob) {
          alpha += alphas_[child_str];
        } else {
          // child's prob shouldn't be less than parent's prob. it should not have
          // been added to leads_to_ in the first place
          assert(false);
        }
      }
    } else {
      ++alpha;
    }
  }
  alphas_[s] = alpha;
  return alpha;
}

void ProbVerifier::addChild(const GlobalState* par, const GlobalState* child) {
  string p_str = par->toString();
  string c_str = child->toString();
  if (leads_to_.find(p_str) == leads_to_.end())
    leads_to_[p_str] = vector<string>();
  leads_to_[p_str].push_back(c_str);
}

void ProbVerifier::addSTOP(StoppingState* rs) {
  _stops.push_back(rs);
}

void ProbVerifier::addEND(StoppingState *end) {
  _ends.push_back(end);
}

void ProbVerifier::printSeq(const vector<GlobalState*>& seq) {
  if (!seq.size())
    return;
  for (int ii = 0 ; ii < (int)seq.size()-1 ; ++ii) {
    if (seq[ii]->getProb() != seq[ii+1]->getProb()) {
      int d = seq[ii+1]->getProb() - seq[ii]->getProb();
      if (d == 1)
        cout << seq[ii]->toString() << " -p->" << endl;
      else
        cout << seq[ii]->toString() << " -p" << d << "->" << endl;
    }
    else
      cout << seq[ii]->toString() << " -> " << endl;
#ifdef DEBUG
    if (isError(seq[ii])) {
      cout << endl;
      cout << "Error state found in sequence: " << seq[ii]->toString() << endl;
    }
#endif
  }
  cout << seq.back()->toString() << endl;
}

bool ProbVerifier::isError(GlobalState* obj)
{
    bool result = true;
    if( _checker != 0 ) {
        result = _checker->check(obj->chkState(), obj->getStateVec()) ;
    }
    return ( !result || findMatch(obj, _errors) );
}

bool ProbVerifier::isStopping(const GlobalState* obj) {
  bool result = findMatch(obj, _stops);
  if (result)
    reached_stoppings_.insert(obj->toString());
  return result;
}

bool ProbVerifier::isEnding(const GlobalState *obj) {
  bool result = findMatch(obj, _ends);
  if (result)
    reached_endings_.insert(obj->toString());
  return result;
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

void ProbVerifier::resetStat() {
  num_transitions_ = 0;
}

void ProbVerifier::printStat() {
  int n = 0;
  for (const auto& c : classes_)
    n += c.size();
  cout << n << " reachable states found." << endl ;
  n = 0;
  cout << reached_stoppings_.size() << " stopping states discovered." << endl;
  cout << reached_endings_.size() << " ending states discovered." << endl;
  cout << num_transitions_ << " transitions taken." << endl;
}

void ProbVerifier::printStat(int class_k) {
  cout << classes_[class_k].size() << " reachable states in class[" 
       << class_k << "]" << endl;
  cout << entries_[class_k].size() << " entry points discovered in entry["
       << class_k << "]" << endl;
  int alpha = 0;
  for(auto pr : explored_entries_[class_k]) {
    GlobalState* gs = pr.second;
    if (gs->getPathCount() > alpha)
      alpha = gs->getPathCount();
  }
  cout << "alpha (maximum of path counts of all entry point states) = " << alpha
       << endl;
}

void ProbVerifier::printStoppings() {
  cout << "Stopping states reached:" << endl;
  for (const auto& s : reached_stoppings_)
    cout << s << endl;
}

void ProbVerifier::printEndings() {
  cout << "Ending states reached:" << endl;
  for (const auto& s : reached_endings_)
    cout << s << endl;
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

bool ProbVerifier::hasProgress(GlobalState* gs) {
  int in_cycle = 0;
  string str_cycle_start = gs->toString();
  for (auto s : dfs_stack_state_) {
    if (in_cycle && isStopping(s)) {
      return true;
    } else if (s->toString() == str_cycle_start) {
      in_cycle = 1;
    }
  }
  return false;
}

void ProbVerifier::reportDeadlock(GlobalState* gs) {
  cout << "Deadlock found." << endl ;
  printStat() ;
#ifdef TRACE
  dfs_stack_state_.front()->printTrail();
  stackPrint();
#endif
  throw ProtocolError::kDeadLock;
}

void ProbVerifier::reportLivelock(GlobalState* gs) {
  cout << "Livelock found after " << gs->getProb()
       << " low probability transitions" << endl ;
  printStat();
#ifdef TRACE
  dfs_stack_state_.front()->printTrail();
  stackPrintUntil(gs->toString());
  cout << "entering cycle:" << endl;
  stackPrintFrom(gs->toString());
  cout << "-> " << gs->toString() << endl;
#endif
  throw ProtocolError::kLivelock;
}

void ProbVerifier::reportError(GlobalState* gs) {
  cout << "Error state found: " << gs->toString() << endl;
  printStat() ;
#ifdef TRACE
  dfs_stack_state_.front()->printTrail();
  stackPrint();
#endif
  throw ProtocolError::kErrorState;
}

bool ProbVerifier::isMemberOf(const GlobalState* gs,
                              const vector<string>& container) {
  return isMemberOf(gs->toString(), container);
}

bool ProbVerifier::isMemberOf(const string& s,
                              const vector<string>& container) {
  for (const auto& str : container)
    if (s == str)
      return true;
  return false;
}

GlobalState* ProbVerifier::isMemberOf(const GlobalState* gs,
                                      const GSClass& container) {
  return isMemberOf(gs->toString(), container);
}

GlobalState* ProbVerifier::isMemberOf(const string& s,
                                      const GSClass& container) {
  auto it = container.find(s);
  if (it == container.end())
    return nullptr;
  else
    return it->second;
}

GlobalState* ProbVerifier::isMemberOf(const GlobalState* gs,
                                      const vector<GSClass>& containers) {
  return isMemberOf(gs->toString(), containers);
}

GlobalState* ProbVerifier::isMemberOf(const string& s,
                                      const vector<GSClass>& containers) {
  for (const auto& c : containers) {
    auto ret = isMemberOf(s, c);
    if (ret)
      return ret;
  }
  return nullptr;
}

GlobalState* ProbVerifier::isMemberOfClasses(const GlobalState* gs) {
  return isMemberOf(gs, classes_);
}

GlobalState* ProbVerifier::isMemberOfClasses(const string& s) {
  return isMemberOf(s, classes_);
}

GlobalState* ProbVerifier::isMemberOfEntries(const GlobalState* gs) {
  return isMemberOf(gs, entries_);
}

GlobalState* ProbVerifier::isMemberOfEntries(const string& s) {
  return isMemberOf(s, entries_);
}

GlobalState* ProbVerifier::copyToClass(const GlobalState* gs, int k) {
  return classes_[k][gs->toString()] = new GlobalState(gs);
}

GlobalState* ProbVerifier::copyToEntry(const GlobalState* gs, int k) {
  if (verbosity_ >= 7) {
    for (int i = 0; i < dfs_stack_state_.size() + 1; ++i)
      cout << "  ";
    cout << dfs_stack_state_.size() + 1;
    cout << "-> entry reached " << gs->toString()
         << " Prob = " << gs->getProb() << endl;
  }
  return entries_[k][gs->toString()] = new GlobalState(gs);
}

GlobalState* ProbVerifier::copyToExploredEntry(const GlobalState* gs, int k) {
  return explored_entries_[k][gs->toString()] = new GlobalState(gs);
}

void ProbVerifier::stackPush(GlobalState *gs) {
  assert(dfs_stack_state_.size() == dfs_stack_string_.size());
  dfs_stack_state_.push_back(gs);
  dfs_stack_string_.push_back(gs->toString());
}

void ProbVerifier::stackPop() {
  assert(dfs_stack_state_.size() == dfs_stack_string_.size());
  dfs_stack_state_.pop_back();
  dfs_stack_string_.pop_back();
}

void ProbVerifier::stackPrint() {
  for (auto s : dfs_stack_state_)
    printArrowStateNewline(s);
}

void ProbVerifier::stackPrintFrom(const string& from) {
  int start = 0;
  for (auto s : dfs_stack_state_) {
    if (start) {
      printArrowStateNewline(s);
    } else if (s->toString() == from) {
      start = 1;
      printArrowStateNewline(s);
    }
  }
}

void ProbVerifier::stackPrintUntil(const string& until) {
  for (auto s : dfs_stack_state_) {
    if (s->toString() == until)
      break;
    else
      printArrowStateNewline(s);
  }
}

void ProbVerifier::printArrowStateNewline(const GlobalState* gs) {
  cout << "-> " << gs->toString() << endl;
}

bool ProbVerifier::isMemberOfStack(const GlobalState* gs) {
  return isMemberOf(gs, dfs_stack_string_);
}
