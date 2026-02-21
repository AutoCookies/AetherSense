#pragma once

#include <cstdint>
#include <vector>
#include <string>

namespace aethersense::dsp {

float JitterMetric(const std::vector<std::uint64_t> &timestamps_ns);
std::vector<float> ResampleToUniformGrid(const std::vector<std::uint64_t> &timestamps_ns,
                                         const std::vector<float> &samples,
                                         const std::string &method);

} // namespace aethersense::dsp
