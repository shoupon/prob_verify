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

ProbVerifierConfig::ProbVerifierConfig()
    : low_p_bound_(1000.0), low_p_bound_inverse_(1.0/low_p_bound_),
      bound_method_(DFS_TWO_STEP), trace_back_(1) {
}

void ProbVerifierConfig::setLowProbBound(double p) {
  low_p_bound_ = p;
  low_p_bound_inverse_ = 1.0 / p;
}

void ProbVerifierConfig::setBoundMethod(int method) {
  bound_method_ = method;
}

vector<StateMachine*> ProbVerifier::machine_ptrs_;
Indexer<string> ProbVerifier::unique_states_;

ProbVerifier::~ProbVerifier() {
  for (auto& container : entries_) {
    for (auto& g : container)
      delete g.second;
  }
  for (auto& container : explored_entries_) {
    for (auto& g : container)
      delete g.second;
  }
}

void ProbVerifier::addError(StoppingState *es) {
  if (es->getStateVec().size() != machine_ptrs_.size()) {
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

StateMachine* ProbVerifier::getMachine(const string& machine_name) {
  return machine_ptrs_[StateMachine::machineToInt(machine_name) - 1];
}

void ProbVerifier::initialize(const GlobalState* init_state) {
  if (init_state) {
    start_point_ = unique_ptr<GlobalState>(new GlobalState(init_state));
  } else {
    start_point_ = unique_ptr<GlobalState>(new GlobalState(getMachinePtrs()));
    default_stopping_ = unique_ptr<StoppingState>(
      new StoppingState(start_point_.get()));
    for (auto ptr : getMachinePtrs()) {
      StateSnapshot *snapshot = ptr->curState();
      default_stopping_->addAllow(snapshot, ptr->macId() - 1);
      delete snapshot;
    }
    addSTOP(default_stopping_.get());
  }
#ifdef LOG
  cout << "Stopping states:" << endl ;
  for (size_t i = 0 ; i < _stops.size() ; ++i)
    cout << _stops[i]->toString() ;
  cout << endl;
  cout << "Error states:" << endl;
  for (size_t i = 0 ; i < _errors.size() ; ++i)
    cout << _errors[i]->toString() ;
  cout << endl;
#endif

  classes_.clear();
  entries_.clear();
  entries_.resize(1);

  copyToEntry(start_point_.get(), 0);
  assert(isStopping(start_point_.get()));
}

void ProbVerifier::start(int max_class) {
  start(max_class, 1);
}

void ProbVerifier::start(int max_class, const GlobalState* init_state) {
  start(max_class, init_state, 1);
}

void ProbVerifier::start(int max_class, int verbose) {
  start(max_class, nullptr, verbose);
}

void ProbVerifier::start(int max_class, const GlobalState* init_state,
                         int verbose) {
  verbosity_ = verbose;
  initialize(init_state);
  classes_.resize(max_class + 1, GSClass());
  entries_.resize(max_class + 10, GSEntry());
  explored_entries_.resize(max_class + 1, GSEntry());
  try {
    nd_choices_.clear();
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
        if (isMemberOfClasses(s) == -1) {
          copyToClass(s, cur_class);
          DFSVisit(s, cur_class);
          copyToExploredEntry(s, cur_class);
        }
        delete s;
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
      if (findCycle())
        cout << "Cycle found in transition system. Bound may diverge." << endl;
      for (int k = 1; k <= max_class + 1; ++k) {
        double alpha = computeBound(k);
        cout << "Probability of reaching class[" << k << "]"
             << " from the initial state is ";
        if (alpha) {
          cout << "bounded by " << alpha << "p";
          if (k - 1)
            cout << "^" << k;
          cout << "." << endl;
        } else {
          cout << "zero." << endl;
        }
      }
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

double ProbVerifier::computeBound(int target_class) {
  clock_t start_time = clock();
  // start from each entry state of class[0] (stopping state/progressive state)
  // and recursively compute the constant alpha from the furthest edge state in
  // equivalent class class[target_class - 1]
  double ipk = 1.0;
  inverse_ps_.clear();
  for (int i = 0; i < target_class + 10; ++i)
    inverse_ps_.push_back(ipk *= config_.low_p_bound_inverse_);

  stack_depth_ = 0;
  double max_alpha = 0.0;
  alphas_.clear();
  int num_iterations = 0;
  do {
    alpha_diff_ = 0;
    for (auto pair : explored_entries_[0]) {
      assert(visited_.empty());
      double alpha = 0;
      if (config_.bound_method_ == DFS_TWO_STEP ||
          config_.bound_method_ == DFS_ONE_STEP) {
        alpha = DFSComputeBound(pair.first, target_class);
      } else if(config_.bound_method_ == TREE_BOUND ||
                config_.bound_method_ == TREE_CLASSIC) {
        alpha = treeComputeBound(pair.first, 0, target_class);
      } else {
        cerr << "Bounding method undefined. Aborting." << endl;
        return -1;
      }

      visited_.clear();

      if (log_alpha_evaluation_) {
        cout << "bound originated from " << pair.first
             << " = " << alpha << endl;
      }
      if (alpha > max_alpha)
        max_alpha = alpha;
    }
    num_iterations++;
    if (verbosity_) {
      cout << "Iteration " << num_iterations << ": "
           << "total alpha increases by " << alpha_diff_ << endl;
    }
  } while (alpha_diff_ > config_.low_p_bound_ ||
           alpha_diff_ < -config_.low_p_bound_);

  if (verbosity_) {
    cout << num_iterations << " iterations needed for bound convergence."
         << endl;
  }
  clock_t end_time = clock();
  cout << "Running time of bound computation: ";
  cout << (double)(end_time - start_time) / CLOCKS_PER_SEC << " secs" << endl;
  return max_alpha;
}

bool ProbVerifier::findCycle() {
  for (auto& pair : explored_entries_[0]) {
    dfs_stack_indices_.clear();
    visited_.clear();
    if (DFSFindCycle(pair.first))
      return true;
    assert(dfs_stack_indices_.empty());
  }
  return false;
}

void ProbVerifier::DFSVisit(GlobalState* gs, int k) {
  stackPush(gs);
  if (verbosity_ >= 5) {
    for (int i = 0; i < dfs_stack_state_.size(); ++i)
      cout << "  ";
    cout << dfs_stack_state_.size();
    cout << "-> " << gs->toReadable()
         << " Prob = " << gs->getProb() << endl;
  }
  if (classes_[k].size() % PROGRESS_CHUNK == 0)
    cerr << "Finished exploring " << classes_[k].size()
         << " states in class[" << k << "]" << endl;
  if (isError(gs)) {
    reportError(gs);
  }
  vector<GlobalState*> nd_choices;
  gs->findSucc(nd_choices);
  if (!nd_choices.size()) {
    reportDeadlock(gs);
  }

  for (auto choice : nd_choices) {
    NonDetChoice non_det_choice;
    for (auto child_ptr : choice->getChildren()) {
      num_transitions_++;
      int p = child_ptr->getProb() - gs->getProb();
      int child_idx = isMemberOfClasses(child_ptr);
      if (p) {
        if (child_idx < 0) {
          // unexplored entry state in higher classes
          if (config_.trace_back_)
            child_ptr->setTrail(dfs_stack_indices_);
          copyToEntry(child_ptr, k + p);
        } else {
          // low probability successor identical to some explored state
        }
        addProbChoice(non_det_choice, child_ptr, p);
      } else if (isMemberOfStack(child_ptr)) {
        // found cycle
        if (!hasProgress(child_ptr)) {
          if (!isStopping(child_ptr))
            reportLivelock(child_ptr);
        }
      } else if (child_idx < 0) {
        // high probability successor is an unexplored state
        if (isEnding(child_ptr)) {
          copyToClass(child_ptr, k);
          child_ptr->setProb(k);
          if (verbosity_ >= 6)
            cout << "Ending state reached. " << endl;
        } else if (!k && isStopping(child_ptr)) {
          // discover new stopping state/entry point in probability class[0]
          if (config_.trace_back_)
            child_ptr->setTrail(dfs_stack_indices_);
          copyToEntry(child_ptr, k);
        } else {
          DFSVisit(child_ptr, k);
          copyToClass(child_ptr, k);
          addProbChoice(non_det_choice, child_ptr);
        }
      } else {
        // high probability successor is an explored state
        if (!isStopping(child_ptr))
          addProbChoice(non_det_choice, child_ptr);
      }
    } // end for all children under a non-deterministic choice
    addNonDetChoice(gs, non_det_choice);
  } // end for all non-deterministic choices
  for (auto choice : nd_choices) {
    for (auto c : choice->getChildren())
      delete c;
    delete choice;
  }
  stackPop();
}

void printIndent(int n) {
  int k = n;
  while (k--)
    cout << "  ";
  cout << n << ": ";
}

double ProbVerifier::DFSComputeBound(int state_idx, int limit) {
  ++stack_depth_;
  if (log_alpha_evaluation_) {
    printIndent(stack_depth_);
    cout << "evaluating alpha of " << indexToState(state_idx) << endl;
  }
  if (nd_choices_.find(state_idx) == nd_choices_.end()) {
    --stack_depth_;
    return alphas_[state_idx] = 0;
  }
  double max_over_nd_choices = 0.0;
  for (const auto& ndc : nd_choices_[state_idx]) {
    double alpha = 0;
    double max_alpha = 0;
    int num_low_prob = 0;
    double low_prob_alphas = 0;
    double even_low_prob = 0;
    int parent_prob = isMemberOfClasses(state_idx);
    for (const auto& trans : ndc.prob_choices) {
      int p = trans.probability_;
      int child_idx = trans.state_idx_;
      auto child_prob = isMemberOfClasses(child_idx);
      if (log_alpha_evaluation_) {
        printIndent(stack_depth_);
        cout << "evaluating child " << indexToState(trans.state_idx_)
             << " having transition probability " << p << endl;
      }

      if (child_prob < 0) {
        if (parent_prob + p == limit) {
          ++num_low_prob;
        } else {
          even_low_prob += (1.0 / inverse_ps_[p - 2]);
        }
      } else if (child_prob >= limit) {
        if (parent_prob + p == limit && child_prob == limit) {
          ++num_low_prob;
        } else {
          even_low_prob += (1.0 / inverse_ps_[p - 2]);
        }
      } else {
        if ((!p && child_prob == parent_prob) ||
            (p && child_prob == parent_prob + p)) {
          double child_alpha;
          if (visited_.find(child_idx) == visited_.end()) {
            child_alpha = DFSComputeBound(child_idx, limit);
            visited_.insert(child_idx);
          } else {
            child_alpha = alphas_[child_idx];
          }

          if (log_alpha_evaluation_) {
            printIndent(stack_depth_);
            cout << indexToState(child_idx) << "'s alpha = "
                 << child_alpha << endl;
          }
          if (!p) {
            if (child_prob == parent_prob) {
              if (child_alpha > max_alpha)
                max_alpha = child_alpha;
            } else if (child_prob > parent_prob)
              assert(false);
          } else {
            if (child_prob == parent_prob + p) {
              low_prob_alphas += child_alpha;
            } else if (child_prob > parent_prob + p) {
              assert(false);
            }
          }
        } else {
          if (alphas_.find(child_idx) != alphas_.end()) {
            double child_alpha = alphas_[child_idx];
            even_low_prob +=
                (child_alpha / inverse_ps_[parent_prob - child_prob + p - 1]);
          }
        }
      }
    } // for probabilistic choices under a non-deterministic choice of 
      // state_idx
    if (log_alpha_evaluation_) {
      printIndent(stack_depth_);
      cout << indexToState(state_idx) << "'s alpha = "
           << alpha << "." << endl;
      printIndent(stack_depth_);
      cout << max_alpha << " comes from states within the same class." << endl;
      printIndent(stack_depth_);
      cout << low_prob_alphas
           << " comes from the computed alphas of states one class deeper."
           << endl;
      printIndent(stack_depth_);
      cout << num_low_prob << " comes from the unknown states beyond the limit."
           << endl;
      printIndent(stack_depth_);
      cout << even_low_prob
           << " comes from states that are more than one class deeper." << endl;
    }

    alpha = low_prob_alphas + num_low_prob + max_alpha;
    even_low_prob =
        ceil(even_low_prob * config_.low_p_bound_inverse_) * config_.low_p_bound_;
    alpha += even_low_prob;
    if (config_.bound_method_ == DFS_TWO_STEP) {
      if (alpha > max_over_nd_choices)
        max_over_nd_choices = alpha;
    } else if (config_.bound_method_ == DFS_ONE_STEP) {
      max_over_nd_choices += alpha;
    } else {
      assert(false);
    }
  } // for non-deterministic choices of state_idx

  --stack_depth_;
  if (alphas_.find(state_idx) == alphas_.end()) {
    alpha_diff_ += max_over_nd_choices;
  } else {
    double old_alpha = alphas_[state_idx];
    if (old_alpha != max_over_nd_choices) {
      assert(old_alpha < max_over_nd_choices);
      alpha_diff_ += (max_over_nd_choices - old_alpha);
    }
  }
  return alphas_[state_idx] = max_over_nd_choices;
}

int ProbVerifier::treeComputeBound(int state_idx, int depth, int limit) {
  assert(depth < limit);

  int alpha = 0;
  int max_alpha = 0;
  int num_low_prob = 0;
  int low_prob_alphas = 0;
  double even_low_prob = 0;

  for (const auto& ndc : nd_choices_[state_idx]) {
    for (const auto& trans : ndc.prob_choices) {
      int p = trans.probability_;
      int child_idx = trans.state_idx_;

      if (depth + p >= limit) {
        if (p == 1)
          ++num_low_prob;
        else
          even_low_prob += (1.0 / inverse_ps_[p - 2]);
      } else {
        int a = treeComputeBound(child_idx, depth + p, limit);
        if (!p) {
          if (a > max_alpha)
            max_alpha = a;
        } else if (p == 1) {
          low_prob_alphas += a;
        } else if (p > 1) {
          even_low_prob += ((double)a) / inverse_ps_[p - 2] ;
        }
      }
    }
  }

  alpha = low_prob_alphas + num_low_prob + max_alpha;
  if (even_low_prob > 0.0)
    alpha += (int)even_low_prob + 1;
  return alpha;
}

bool ProbVerifier::DFSFindCycle(int state_idx) {
  dfs_stack_indices_.push_back(state_idx);
  if (nd_choices_.find(state_idx) != end(nd_choices_)) {
    for (const auto& ndc : nd_choices_[state_idx]) {
      for (const auto& t : ndc.prob_choices) {
        if (isMemberOf(t.state_idx_, dfs_stack_indices_))
          return true;
        if (visited_.find(t.state_idx_) == visited_.end()) {
          if (DFSFindCycle(t.state_idx_))
            return true;
        }
      }
    }
  }
  dfs_stack_indices_.pop_back();
  return false;
}

void ProbVerifier::addProbChoice(NonDetChoice &ndc, const GlobalState *child) {
  addProbChoice(ndc, child, 0);
}

void ProbVerifier::addProbChoice(NonDetChoice &ndc, const GlobalState *child,
                                 int prob) {
  int c_idx = stateToIndex(child);
  ndc.prob_choices.push_back(ProbChoice(c_idx, prob));
}

void ProbVerifier::addNonDetChoice(const GlobalState *parent,
                                   const NonDetChoice &ndc) {
  int p_idx = stateToIndex(parent);
  if (nd_choices_.find(p_idx) == end(nd_choices_))
    nd_choices_[p_idx] = vector<NonDetChoice>();
  nd_choices_[p_idx].push_back(ndc);
}

void ProbVerifier::addMachine(StateMachine* machine) {
  int mac_id = StateMachine::machineToInt(machine->getName());
  if (mac_id > machine_ptrs_.size())
    machine_ptrs_.resize(mac_id);
  machine_ptrs_[mac_id - 1] = machine;
  cout << "Machine " << machine->getName() << " is added at index "
       << mac_id - 1 << endl;
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
    reached_stoppings_.insert(stateToIndex(obj));
  return result;
}

bool ProbVerifier::isEnding(const GlobalState *obj) {
  bool result = findMatch(obj, _ends);
  if (result)
    reached_endings_.insert(stateToIndex(obj));
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
        cout << childNode->toString()
             << " Prob = " << prob << endl;
    }
}

void ProbVerifier::resetStat() {
  num_transitions_ = 0;
}

void ProbVerifier::printContent(const GSClass& container) {
  for (const auto& p : container)
    cout << "  " << indexToState(p) << endl;
}

void ProbVerifier::printContent(const vector<GSClass>& containers) {
  for (const auto& c : containers)
    printContent(c);
}

void ProbVerifier::printContent(const GSEntry& container) {
  for (const auto& p : container)
    cout << "  " << indexToState(p.first) << endl;
}

int sumTrailTotal(const GSEntry& container) {
  int sum = 0;
  for (auto entry : container)
    sum += entry.second->getTrailSize();
  return sum;
}
    
void ProbVerifier::printStat() {
  int n = 0;
  for (const auto& c : classes_) {
    n += c.size();
  }

  cout << n << " reachable states found." << endl ;
  cout << reached_stoppings_.size() << " stopping states discovered." << endl;
  cout << reached_endings_.size() << " ending states discovered." << endl;
  cout << num_transitions_ << " transitions taken." << endl;

  size_t total_trail_size = 0;
  for (const auto& c : entries_)
    total_trail_size += sumTrailTotal(c);
  cout << total_trail_size
       << " states accrued in trails for obtaining witness/counterexamples."
       << endl;

  size_t transitions_bytes = sizeof(unordered_map<int, vector<NonDetChoice>>);
  cout << "size of unordered_map = "
       << sizeof(unordered_map<int, vector<NonDetChoice>>) << " bytes." << endl;
  for (const auto &p : nd_choices_) {
    for (auto t : p.second)
      transitions_bytes += sizeof(ProbChoice);
  }
  cout << transitions_bytes
       << " bytes are used to store the structure of the graph." << endl;
  cout << unique_states_.getBytes() << " bytes are used to store the string "
       << "representations and their corresponding indices." << endl;
}

void ProbVerifier::printStat(int class_k) {
  cout << classes_[class_k].size() << " reachable states in class["
       << class_k << "]" << endl;
  if (verbosity_ >= 4) {
    cout << "The states in class[" << class_k << "] are: " << endl;
    printContent(classes_[class_k]);
  }
  cout << entries_[class_k].size() << " entry points discovered in entry["
       << class_k << "]" << endl;
  cout << sumTrailTotal(entries_[class_k])
       << " states accrued for trails for obtaining witness/counterexamples"
       << " in entry[" << class_k << "]." << endl;
  if (verbosity_ >= 4) {
    cout << "The states in entry[" << class_k << "] are: " << endl;
    printContent(entries_[class_k]);
  }
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

vector<StateMachine*> ProbVerifier::getMachinePtrs() const {
  return machine_ptrs_;
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
  cout << "Deadlock found: " << gs->toReadableMachineName() << endl ;
  printStat() ;
  if (config_.trace_back_) {
    dfs_stack_state_.front()->printTrail(indexToState);
    stackPrint();
  }
  throw ProtocolError::kDeadLock;
}

void ProbVerifier::reportLivelock(GlobalState* gs) {
  cout << "Livelock found after " << gs->getProb()
       << " low probability transitions" << endl ;
  printStat();
  if (config_.trace_back_) {
    dfs_stack_state_.front()->printTrail(indexToState);
    stackPrintUntil(gs->toString());
    cout << "entering cycle:" << endl;
    stackPrintFrom(gs->toString());
    cout << "-> " << gs->toReadableMachineName() << endl;
  }
  throw ProtocolError::kLivelock;
}

void ProbVerifier::reportError(GlobalState* gs) {
  cout << "Error state found: " << gs->toReadableMachineName() << endl;
  printStat() ;
  if (config_.trace_back_) {
    dfs_stack_state_.front()->printTrail(indexToState);
    stackPrint();
  }
  throw ProtocolError::kErrorState;
}

int ProbVerifier::stateToIndex(const string& state_str) {
  return unique_states_.insert(state_str);
}

int ProbVerifier::stateToIndex(const GlobalState* gs) {
  return stateToIndex(gs->toString());
}

string ProbVerifier::indexToState(int idx) {
  return unique_states_.getItem(idx);
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

bool ProbVerifier::isMemberOf(const GlobalState* gs,
                              const vector<int>& container) {
  return isMemberOf(stateToIndex(gs), container);
}

bool ProbVerifier::isMemberOf(int state_idx,
                              const vector<int>& container) {
  for (auto i : container)
    if (i == state_idx)
      return true;
  return false;
}

bool ProbVerifier::isMemberOf(const GlobalState* gs,
                              const GSClass& container) {
  return isMemberOf(stateToIndex(gs), container);
}

bool ProbVerifier::isMemberOf(const string& s,
                              const GSClass& container) {
  return isMemberOf(stateToIndex(s), container);
}

bool ProbVerifier::isMemberOf(int state_idx,
                              const GSClass& container) {
  auto it = container.find(state_idx);
  return it != end(container);
}

bool ProbVerifier::isMemberOf(const GlobalState *gs,
                              const GSEntry& container) {
  return isMemberOf(stateToIndex(gs), container);
}

bool ProbVerifier::isMemberOf(const string& s,
                              const GSEntry& container) {
  return isMemberOf(stateToIndex(s), container);
}

bool ProbVerifier::isMemberOf(int state_idx,
                              const GSEntry& container) {
  auto it = container.find(state_idx);
  return it != end(container);
}

int ProbVerifier::isMemberOf(const GlobalState* gs,
                             const vector<GSClass>& containers) {
  return isMemberOf(stateToIndex(gs), containers);
}

int ProbVerifier::isMemberOf(const string& s,
                             const vector<GSClass>& containers) {
  return isMemberOf(stateToIndex(s), containers);
}

int ProbVerifier::isMemberOf(int state_idx,
                            const vector<GSClass>& containers) {
  for (int prob = 0; prob < containers.size(); ++prob) {
    auto found = isMemberOf(state_idx, containers[prob]);
    if (found)
      return prob;
  }
  return -1;
}

int ProbVerifier::isMemberOf(const GlobalState* gs,
                             const vector<GSEntry>& containers) {
  return isMemberOf(stateToIndex(gs), containers);
}

int ProbVerifier::isMemberOf(const string& s,
                             const vector<GSEntry>& containers) {
  return isMemberOf(stateToIndex(s), containers);
}

int ProbVerifier::isMemberOf(int state_idx,
                             const vector<GSEntry>& containers) {
  for (int prob = 0; prob < containers.size(); ++prob) {
    auto found = isMemberOf(state_idx, containers[prob]);
    if (found)
      return prob;
  }
  return -1;
}

int ProbVerifier::isMemberOfClasses(const GlobalState* gs) {
  return isMemberOf(gs, classes_);
}

int ProbVerifier::isMemberOfClasses(const string& s) {
  return isMemberOf(s, classes_);
}

int ProbVerifier::isMemberOfClasses(int state_idx) {
  return isMemberOf(state_idx, classes_);
}

int ProbVerifier::isMemberOfEntries(const GlobalState* gs) {
  return isMemberOf(gs, entries_);
}

int ProbVerifier::isMemberOfEntries(const string& s) {
  return isMemberOf(s, entries_);
}

int ProbVerifier::isMemberOfEntries(int state_idx) {
  return isMemberOf(state_idx, entries_);
}

void ProbVerifier::copyToClass(const GlobalState* gs, int k) {
  int idx = stateToIndex(gs);
  if (classes_[k].find(idx) == end(classes_[k]))
    classes_[k].insert(idx);
}

void ProbVerifier::copyToEntry(const GlobalState* gs, int k) {
  int idx = stateToIndex(gs);
  if (entries_[k].find(idx) == end(entries_[k]))
    entries_[k][idx] = new GlobalState(gs);
  if (verbosity_ >= 7) {
    for (int i = 0; i < dfs_stack_state_.size() + 1; ++i)
      cout << "  ";
    cout << dfs_stack_state_.size() + 1;
    cout << "-> entry reached " << gs->toString()
         << " Prob = " << gs->getProb() << endl;
  }
}

void ProbVerifier::copyToExploredEntry(const GlobalState* gs, int k) {
  int idx = stateToIndex(gs);
  if (explored_entries_[k].find(idx) == end(explored_entries_[k]))
    explored_entries_[k][idx] = new GlobalState(gs);
}

void ProbVerifier::stackPush(GlobalState *gs) {
  assert(dfs_stack_state_.size() == dfs_stack_indices_.size());
  dfs_stack_state_.push_back(gs);
  dfs_stack_indices_.push_back(stateToIndex(gs));
}

void ProbVerifier::stackPop() {
  assert(dfs_stack_state_.size() == dfs_stack_indices_.size());
  dfs_stack_state_.pop_back();
  dfs_stack_indices_.pop_back();
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
  cout << "-> " << gs->toReadable() << endl;
}

bool ProbVerifier::isMemberOfStack(const GlobalState* gs) {
  return isMemberOf(gs, dfs_stack_indices_);
}
