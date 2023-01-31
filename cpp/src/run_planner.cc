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

#include <iostream>
#include <memory>
#include <optional>

#include "heuristics/novelty.h"
#include "heuristics/recursive_graph_distance.h"
#include "heuristics/weighted_sum.h"
#include "pushworld_puzzle.h"
#include "search/best_first_search.h"
#include "search/priority_queue.h"
#include "search/search.h"

namespace pushworld {

/**
 * Solves the given puzzle using best-first search with a heuristic determined
 * by the mode. Supported modes include:
 *
 *      "RGD": The recursive graph distance heuristic.
 *      "N+RGD": A lexicographic combination of the novelty heuristic followed
 * by the recursive graph distance heuristic.
 */
std::optional<Plan> solve(const std::shared_ptr<PushWorldPuzzle> puzzle,
                          const std::string& mode) {
  priority_queue::FibonacciPriorityQueue<std::shared_ptr<search::SearchNode>,
                                         float>
      frontier;
  auto rgd =
      std::make_shared<heuristic::RecursiveGraphDistanceHeuristic>(puzzle);

  if (mode == "RGD") {
    return best_first_search(*puzzle, *rgd, frontier);
  } else if (mode == "N+RGD") {
    heuristic::HeuristicsAndWeights heuristics_and_weights = {
        {std::make_shared<heuristic::NoveltyHeuristic>(
             puzzle->getInitialState().size()),
         // The maximum novelty is 3, so 3e6 maintains sub-integer precision
         // with a `float` type. All RGD heuristic values are either integers
         // or infinite.
         1e6f},
        {rgd, 1.0f}};
    heuristic::WeightedSumHeuristic heuristic(heuristics_and_weights);
    return best_first_search(*puzzle, heuristic, frontier);
  } else {
    throw std::domain_error("Unrecognized mode: " + mode);
  }
}

}  // namespace pushworld

/**
 * Solves a given PushWorld puzzle and prints the resulting solution, if one
 * exists.
 */
int main(int argc, char* argv[]) {
  try {
    if (argc != 3) {
      std::cout
          << ("Usage: run_planner <mode> <puzzle>\n\n"
              "Prints a plan of (L)eft, (R)ight, (U)p, (D)own actions that "
              "solve the given PushWorld puzzle, or prints \"NO SOLUTION\" "
              "if no solution exists.\n\n"
              "Options:\n"
              "    <mode>    : \"RGD\"   - The recursive graph distance "
              "heuristic.\n"
              "                \"N+RGD\" - A lexicographic combination of the "
              "novelty heuristic with the RGD heuristic.\n"
              "    <puzzle> : The path of a PushWorld file in .pwp format.\n\n");
      return 0;
    }

    const auto puzzle = std::make_shared<pushworld::PushWorldPuzzle>(argv[2]);
    const auto plan = pushworld::solve(puzzle, argv[1]);

    if (plan == std::nullopt) {
      std::cout << "NO SOLUTION\n";
    } else {
      for (const pushworld::Action& action : *plan) {
        std::cout << pushworld::ACTION_TO_CHAR[action];
      }
      std::cout << "\n";
    }
  } catch (std::exception& e) {
    std::cerr << "ERROR: " << e.what() << "\n";
    return 1;
  } catch (...) {
    std::cerr << "UNKNOWN ERROR\n";
  }
  return 0;
}
