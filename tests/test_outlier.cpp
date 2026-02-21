#include "test_harness.hpp"

#include "aethersense/dsp/outlier.hpp"

TEST_CASE(Outlier_filter_replaces_spike) {
  std::vector<float> v{1,1,10,1,1};
  aethersense::dsp::FilterOutliers(v, "mad", 3.0F, 5);
  REQUIRE(v[2] < 5.0F);
}
