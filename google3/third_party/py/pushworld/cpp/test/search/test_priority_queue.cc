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
#include <string>
#include <vector>

#include "search/priority_queue.h"

namespace priority_queue {

BOOST_AUTO_TEST_SUITE(priority_queue)

/* Checks all `PriorityQueue` implementations. */
BOOST_AUTO_TEST_CASE(test_priority_queue) {
  std::vector<std::shared_ptr<PriorityQueue<std::string, int>>> queues{
      std::make_shared<FibonacciPriorityQueue<std::string, int>>(),
      std::make_shared<BucketPriorityQueue<std::string, int>>()};

  for (auto& queue : queues) {
    BOOST_TEST(queue->empty());
    BOOST_TEST(queue->size() == 0);

    queue->push("foo", 1);
    queue->push("bar", 2);
    queue->push("foo", 3);
    queue->push("baz", 2);

    BOOST_TEST(!queue->empty());
    BOOST_TEST(queue->size() == 4);

    BOOST_TEST(queue->top() == "foo");
    BOOST_TEST(queue->min_priority() == 1);

    queue->pop();
    BOOST_TEST(queue->size() == 3);

    const auto elem = queue->top();
    BOOST_TEST((elem == "baz" or elem == "bar"));
    BOOST_TEST(queue->min_priority() == 2);

    queue->pop();
    BOOST_TEST(queue->size() == 2);

    const auto other_elem = queue->top();
    BOOST_TEST((other_elem == "baz" or other_elem == "bar"));
    BOOST_TEST(elem != other_elem);
    BOOST_TEST(queue->min_priority() == 2);

    queue->pop();
    BOOST_TEST(queue->size() == 1);

    BOOST_TEST(queue->top() == "foo");
    BOOST_TEST(queue->min_priority() == 3);

    queue->clear();
    BOOST_TEST(queue->empty());
    BOOST_TEST(queue->size() == 0);
  }
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace priority_queue
