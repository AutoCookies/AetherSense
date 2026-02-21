#include "test_harness.hpp"

#include <filesystem>
#include <fstream>

#include "aethersense/io/stream_reader.hpp"

TEST_CASE(Stream_reader_reads_file_mode_records) {
  const std::string p = "stream_test.log";
  std::ofstream out(p);
  out << "a\n";
  out << "b\n";
  out.close();

  aethersense::Config::Io io;
  io.mode = "file";
  io.path = p;
  auto r = aethersense::io::CreateStreamReader(io);
  REQUIRE(r.ok());
  REQUIRE(r.value()->open(p).ok());
  auto rec1 = r.value()->read_next();
  REQUIRE(rec1.ok());
  REQUIRE(rec1.value().line == "a");
  std::filesystem::remove(p);
}
