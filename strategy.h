//
//  strategy.h
//  verification
//
//  Created by Shou-pon Lin on 7/28/15.
//  Copyright (c) 2015 Shou-pon Lin. All rights reserved.
//

#ifndef PROBVERIFY_STRATEGY_H
#define PROBVERIFY_STRATEGY_H

#include <iostream>
#include <string>
#include <vector>
using namespace std;

#include "statemachine.h"

class Strategy;

class StrategyState {
  friend Strategy;
public:
  StrategyState() {}
  virtual ~StrategyState() {}
  virtual StrategyState* clone() const { return new StrategyState(); }
  virtual string toString() { return string(); }
  virtual string toReadable() { return toString(); }
};

/*
 * Strategy controls whether a null input transition is valid
 */
class Strategy {
public:
  Strategy() {}
  virtual ~Strategy() {}
  virtual bool validChoice(int mac_id, int null_input_idx,
                           const vector<StateSnapshot*>& mstate,
                           StrategyState *strategy_state) {
    return true;
  }
  virtual StrategyState* createInitState() { return new StrategyState(); }
  virtual void finalizeTransition(const vector<StateSnapshot*>& mstate,
                                  StrategyState *strategy_state) {}
};

#endif /* defined(PROBVERIFY_STRATEGY_H) */
