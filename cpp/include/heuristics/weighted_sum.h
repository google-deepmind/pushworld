/*
 * Copyright 2022 DeepMind Technologies Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef HEURISTICS_WEIGHTED_SUM_H_
#define HEURISTICS_WEIGHTED_SUM_H_

#include <memory>
#include <utility>  // pair
#include <vector>

#include "heuristics/heuristic.h"
#include "pushworld_puzzle.h"

namespace pushworld {
namespace heuristic {

using HeuristicsAndWeights =
    std::vector<std::pair<std::shared_ptr<Heuristic<float>>, float>>;

/**
 * Computes a weighted sum of multiple heuristics.
 */
class WeightedSumHeuristic : public Heuristic<float> {
 private:
  HeuristicsAndWeights m_heuristics_and_weights;

 public:
  /**
   * Constructs this heuristic from a list of (heuristic, weight) pairs.
   */
  WeightedSumHeuristic(const HeuristicsAndWeights& heuristics_and_weights);

  /**
   * Returns the weighted sum of the costs of each heuristic provided to
   * the constructor.
   */
  float estimate_cost_to_goal(const RelativeState& relative_state) override;
};

}  // namespace heuristic
}  // namespace pushworld

#endif /* HEURISTICS_WEIGHTED_SUM_H_ */
