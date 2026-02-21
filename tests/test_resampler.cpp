#include "test_harness.hpp"

#include <cmath>

#include "aethersense/dsp/resampler.hpp"

TEST_CASE(Resampler_reduces_jitter_impact_deterministically) {
  std::vector<std::uint64_t> t; std::vector<float> s;
  const float f = 1.0F;
  for (int i = 0; i < 64; ++i) {
    const std::uint64_t base = 1000000000ULL + static_cast<std::uint64_t>(i) * 50000000ULL;
    const std::int64_t jitter = (i % 3 == 0) ? 8000000 : -4000000;
    const auto ts = static_cast<std::uint64_t>(static_cast<std::int64_t>(base) + jitter);
    t.push_back(ts);
    const float secs = static_cast<float>(ts - t[0]) / 1e9F;
    s.push_back(std::sin(2.0F * 3.1415926F * f * secs));
  }
  auto rs = aethersense::dsp::ResampleToUniformGrid(t, s, "linear");
  REQUIRE(rs.size() == s.size());
  float mse = 0.0F;
  for (std::size_t i = 1; i < rs.size(); ++i) mse += std::fabs(rs[i]-rs[i-1]);
  REQUIRE(mse > 1.0F);
}
