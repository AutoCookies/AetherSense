#include "test_harness.hpp"

#include <vector>

#include "aethersense/runtime/decision_engine.hpp"

TEST_CASE(Decision_hysteresis_and_hold_behavior) {
  aethersense::DecisionEngine engine(1.0F, 0.5F, 2);
  std::vector<float> energy = {0.2F, 1.2F, 0.4F, 0.4F, 0.4F, 1.1F};
  std::vector<bool> states;
  for (float e : energy) {
    states.push_back(engine.Update(e));
  }
  REQUIRE(states[0] == false);
  REQUIRE(states[1] == true);
  REQUIRE(states[2] == true);
  REQUIRE(states[3] == false);
  REQUIRE(states[4] == false);
  REQUIRE(states[5] == true);
}
