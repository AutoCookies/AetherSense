# AetherSense Core (Phase 1)

AetherSense Core is a production-oriented, test-driven C++20 runtime for consent-based **presence/motion detection** from pre-extracted CSI-like complex streams (or generic complex time-series).

## What this is
- Offline-capable CSI-like frame ingestion (CSV/JSONL).
- Strict, versioned core frame and config schemas.
- Deterministic Phase 1 pipeline: normalization (placeholder) -> feature extraction (mean magnitude energy) -> presence decision.
- Thread-safe bounded ring buffer runtime primitive.

## What this is **NOT**
- Not CSI extraction.
- No driver hacking, firmware patching, kernel mods, or packet injection.
- No ML framework integration in Phase 1.

## Quickstart
```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/aethersense_cli --config ./testdata/sample_config.json
```

## Canonical data model (SOT)
`aethersense::CsiFrame` in `include/aethersense/core/types.hpp`.

Required fields:
- `timestamp_ns` (`uint64_t`)
- `center_freq_hz` (`uint64_t`)
- `subcarrier_count` (`uint16_t`)
- `rx_count` (`uint8_t`)
- `tx_count` (`uint8_t`)
- `data` (`std::vector<std::complex<float>>`) row-major mapping:
  `((rx * tx_count + tx) * subcarrier_count + sc)`

Zero-copy `FrameView` is provided with `std::span<const std::complex<float>>`.

## Configuration schema (SOT)
JSON config fields:
- `io.format`: `csv` or `jsonl`
- `io.path`
- `runtime.ring_buffer_capacity_frames` (>= 8)
- `runtime.max_batch_frames` (<= capacity)
- `runtime.clock`: `monotonic` or `from_input`
- `logging.level`
- `pipeline.enabled_stages` (string list)
- `pipeline.presence_threshold` ([0, 1e6])

See `testdata/sample_config.json`.

## Input formats
### CSV
Header required:
```text
timestamp_ns,center_freq_hz,rx,tx,subcarrier_count,data_re,data_im
```
Where `data_re` and `data_im` are semicolon-separated float lists with length `rx*tx*subcarrier_count`.

### JSONL
One JSON object per line with fields:
`timestamp_ns, center_freq_hz, rx, tx, subcarrier_count, data_re, data_im`.

## Testing
```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## CI
GitHub Actions workflow (`.github/workflows/ci.yml`) runs:
- Build/test matrix: Ubuntu (gcc/clang) + macOS (clang)
- clang-format check
- Sanitizer job (ASAN+UBSAN)
- clang-tidy pass

## Roadmap (Phase 2)
- Real normalization stage
- Windowing + filtering
- FFT-domain features
- Multi-metric decision logic
