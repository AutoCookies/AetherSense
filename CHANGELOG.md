# Changelog

## v1.0.0
- Upgraded to config schema v2 with strict validation for DSP, decision, and runtime controls.
- Implemented Phase 2 DSP pipeline: windowed processing, phase unwrap+detrend, smoothing, FFT band-energy features.
- Added deterministic top-K subcarrier selection by variance.
- Added decision hysteresis with hold frames for stable presence state.
- Added runtime metrics (counters + p50/p95 processing latency).
- Added CLI flags: `--version`, `--dry-run`, `--print-config-schema`, `--export-decisions`.
- Expanded deterministic test suite (DSP, backpressure, hysteresis, end-to-end behavior).
