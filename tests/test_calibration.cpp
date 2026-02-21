#include "test_harness.hpp"

#include "aethersense/dsp/calibration.hpp"

TEST_CASE(Calibration_removes_common_phase_error) {
  std::vector<std::vector<float>> phase{{1,2,3},{2,3,4}};
  aethersense::dsp::RemoveCommonPhaseError(phase, true);
  REQUIRE(phase[0][0] < 0.1F);
}
