#pragma once

namespace aethersense {

class DecisionEngine {
public:
  DecisionEngine(float threshold_on, float threshold_off, int hold_frames)
      : threshold_on_(threshold_on), threshold_off_(threshold_off), hold_frames_(hold_frames) {}

  bool Update(float energy) {
    if (state_) {
      if (energy < threshold_off_ && hold_counter_ <= 0) {
        state_ = false;
        hold_counter_ = hold_frames_;
      }
    } else if (energy >= threshold_on_ && hold_counter_ <= 0) {
      state_ = true;
      hold_counter_ = hold_frames_;
    }
    if (hold_counter_ > 0) {
      --hold_counter_;
    }
    return state_;
  }

private:
  float threshold_on_;
  float threshold_off_;
  int hold_frames_;
  bool state_{false};
  int hold_counter_{0};
};

} // namespace aethersense
