#pragma once

#include <string>
#include <vector>

namespace aethersense::dsp {

void FilterOutliers(std::vector<float> &series, const std::string &method, float k, int window);

} // namespace aethersense::dsp
