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

#ifndef SEARCH_PRIORITY_QUEUE_H_
#define SEARCH_PRIORITY_QUEUE_H_

#include <boost/heap/fibonacci_heap.hpp>
#include <stack>
#include <unordered_map>
#include <utility>  // std::pair, std::make_pair

namespace priority_queue {

/**
 * A queue that returns the `Element` with the minimum associated `Priority`.
 *
 * Unlike the `boost::heap` interface, this class explicitly separates elements
 * and priorities, which allows for more efficient storage patterns when either
 * elements or priorities occur multiple times in the same queue.
 *
 * Example usage:
 *
 *     PriorityQueue<string, int> queue;
 *     queue.push("foo", 2);
 *     queue.push("bar", 1);
 *     assert(queue.top() == "bar");
 *     queue.pop();
 *     assert(queue.top() == "foo");
 */
template <typename Element, typename Priority>
class PriorityQueue {
 public:
  /* Returns the number of elements in this queue. */
  virtual size_t size() const = 0;

  /* Returns whether there are no elements in this queue. */
  virtual bool empty() const = 0;

  /* Removes all elements. */
  virtual void clear() = 0;

  /**
   * Adds an element to this queue with the associated priority.
   *
   * The same element can be added multiple times, either with the same or a
   * different priority. This does not replace the existing priority for the
   * element; the queue will contain multiple instances of the same element.
   */
  virtual void push(const Element& element, const Priority& priority) = 0;

  /**
   * Returns the element with the minimum associated priority.
   *
   * Throws an exception if this queue is empty.
   */
  virtual Element top() = 0;

  /**
   * Returns the minimum priority of all elements. This is the priority of the
   * `top` element.
   *
   * Throws an exception if this queue is empty.
   */
  virtual Priority min_priority() = 0;

  /**
   * Removes the node with the lowest priority.
   *
   * Throws an exception if this queue is empty.
   */
  virtual void pop() = 0;
};

namespace {

/* A comparison operator for `boost::heap`. */
template <typename Element, typename Priority>
struct CompareElementPriorityPair {
  bool operator()(const std::pair<Element, Priority>& p1,
                  const std::pair<Element, Priority>& p2) const {
    // This comparison places lowest-priority nodes at the front of a Boost
    // priority queue.
    return p1.second > p2.second;
  }
};

/* A comparison operator for `boost::heap`. */
template <typename Priority>
struct ComparePriority {
  bool operator()(const Priority& p1, const Priority& p2) const {
    // This comparison places lowest-priority nodes at the front of a Boost
    // priority queue.
    return p1 > p2;
  }
};

}  // namespace

/**
 * A priority queue that uses a Fibonacci heap so that `push` and `top` have
 * O(1) complexity and `pop` has O(log N) complexity for N elements in the
 * queue.
 *
 * The `Priority` type must support the `>` comparison operator.
 */
template <typename Element, typename Priority>
class FibonacciPriorityQueue : public PriorityQueue<Element, Priority> {
 private:
  boost::heap::fibonacci_heap<
      std::pair<Element, Priority>,
      boost::heap::compare<CompareElementPriorityPair<Element, Priority>>>
      m_heap;

 public:
  /*
   * Template classes cannot be compiled into libraries without knowing the
   * template types, so to avoid restricting supported template types, all
   * methods in this class are defined in this header file.
   */

  size_t size() const override { return m_heap.size(); };

  bool empty() const override { return m_heap.empty(); };

  void clear() override { m_heap.clear(); };

  void push(const Element& element, const Priority& priority) override {
    m_heap.push(std::make_pair(element, priority));
  };

  Element top() override { return m_heap.top().first; };

  Priority min_priority() override { return m_heap.top().second; };

  void pop() override { m_heap.pop(); };
};

/**
 * A `BucketPriorityQueue` improves the complexity of the
 * `FibonacciPriorityQueue` when multiple elements in the queue have equal
 * priorities.
 *
 * Elements with the same priority are stacked into a "bucket" to reduce the
 * computation in `pop`. If all elements have different priorities, a
 * `BucketPriorityQueue` will be slower than a `FibonacciPriorityQueue`.
 *
 * The `Priority` type must be hashable and must support the `>` comparison
 * operator.
 */
template <typename Element, typename Priority>
class BucketPriorityQueue : public PriorityQueue<Element, Priority> {
 private:
  boost::heap::fibonacci_heap<Priority,
                              boost::heap::compare<ComparePriority<Priority>>>
      m_priority_heap;
  std::unordered_map<Priority, std::stack<Element>> m_elements;
  int m_num_elements;

 public:
  /*
   * Template classes cannot be compiled into libraries without knowing the
   * template types, so to avoid restricting supported template types, all
   * methods in this class are defined in this header file.
   */

  BucketPriorityQueue() : m_num_elements(0){};

  size_t size() const override { return m_num_elements; };

  bool empty() const override { return m_num_elements == 0; };

  void clear() override {
    m_priority_heap.clear();
    m_elements.clear();
    m_num_elements = 0;
  };

  void push(const Element& element, const Priority& priority) override {
    const auto& pair = m_elements.find(priority);

    if (pair == m_elements.end()) {
      // The queue does not already contain this priority.
      m_elements[priority].push(element);
      m_priority_heap.push(priority);
    } else {
      // The queue already contains this priority.
      pair->second.push(element);
    }

    m_num_elements++;
  };

  Element top() override { return m_elements[m_priority_heap.top()].top(); };

  Priority min_priority() override { return m_priority_heap.top(); };

  void pop() override {
    const auto priority = m_priority_heap.top();
    auto& bucket = m_elements[priority];

    if (bucket.size() == 1) {
      m_priority_heap.pop();
      m_elements.erase(priority);
    } else {
      bucket.pop();
    }
    m_num_elements--;
  };
};

}  // namespace priority_queue

#endif /* SEARCH_PRIORITY_QUEUE_H_ */
