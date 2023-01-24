// Copyright 2022 DeepMind Technologies Limited
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <boost/test/unit_test.hpp>
#include <memory>

#include "heuristics/heuristic.h"
#include "heuristics/weighted_sum.h"
#include "pushworld_puzzle.h"

namespace pushworld {
namespace heuristic {

BOOST_AUTO_TEST_SUITE(weighted_sum_heuristic)

namespace {

/* A heuristic that always returns the same cost. */
class ConstantHeuristic : public Heuristic<float> {
 private:
  float m_cost;

 public:
  ConstantHeuristic(const float cost) : m_cost(cost){};
  float estimate_cost_to_goal(const State& state) { return m_cost; }
};

}  // namespace

/* Checks `WeightedSumHeuristic`. */
BOOST_AUTO_TEST_CASE(test_novelty_heuristic) {
  State state;

  for (float i = 0.0f; i < 5; i++) {
    HeuristicsAndWeights heuristics_and_weights = {
        {std::make_shared<ConstantHeuristic>(i), i + 1}};
    WeightedSumHeuristic h(heuristics_and_weights);
    BOOST_TEST(h.estimate_cost_to_goal(state) == i * (i + 1));

    for (float j = -5.0f; j < 5; j++) {
      HeuristicsAndWeights heuristics_and_weights2 = {
          {std::make_shared<ConstantHeuristic>(i), i + 1},
          {std::make_shared<ConstantHeuristic>(j), j + 1}};
      WeightedSumHeuristic h2(heuristics_and_weights2);
      BOOST_TEST(h2.estimate_cost_to_goal(state) == i * (i + 1) + j * (j + 1));
    }
  }
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace heuristic
}  // namespace pushworld
