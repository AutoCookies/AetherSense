#pragma once

#include <vector>

namespace aethersense::dsp {

void RemoveCommonPhaseError(std::vector<std::vector<float>> &phase_by_sc, bool robust_median);
void RemoveLinearTrend(std::vector<float> &series);

} // namespace aethersense::dsp
