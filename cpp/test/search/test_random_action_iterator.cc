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

#include <algorithm>
#include <boost/functional/hash.hpp>
#include <boost/test/unit_test.hpp>
#include <unordered_map>

#include "pushworld_puzzle.h"
#include "search/random_action_iterator.h"

namespace pushworld {
namespace search {

BOOST_AUTO_TEST_SUITE(random_action_iterator)

namespace {

using ActionGroup = std::vector<pushworld::Action>;

struct ActionGroupHash {
  std::size_t operator()(const ActionGroup& action_group) const {
    return boost::hash_range(action_group.begin(), action_group.end());
  }
};

}  // namespace

/**
 * Checks that `RandomActionIterator` generates an approximately uniform
 * distribution of action orders.
 */
BOOST_AUTO_TEST_CASE(test_random_action_iterator) {
  const int num_action_groups = 1000000;
  RandomActionIterator action_iter(num_action_groups);

  std::unordered_map<ActionGroup, int, ActionGroupHash> action_group_counts;

  ActionGroup action_group{0, 1, 2, 3};
  do {
    action_group_counts[action_group] = 0;
  } while (std::next_permutation(action_group.begin(), action_group.end()));

  for (int i = 0; i < num_action_groups; i++) {
    action_group_counts[action_iter.next()]++;
  }

  // There are 4! possible orders of 4 actions.
  const int num_possible_orders = 4 * 3 * 2 * 1;
  BOOST_TEST(action_group_counts.size() == num_possible_orders);

  for (const auto& action_group_count : action_group_counts) {
    // Require that the number of occurrences of each action group is within 10%
    // of a perfectly uniform distribution. For 1,000,000 action groups, the
    // standard deviation of group counts is <1%, so 10% virtually guarantees
    // that this test passes.
    BOOST_TEST(action_group_count.second >
               0.9 * num_action_groups / num_possible_orders);
  }
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace search
}  // namespace pushworld
