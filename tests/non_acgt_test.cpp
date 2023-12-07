#include "test_util.hpp"
#include <cobs/util/fs.hpp>
#include <gtest/gtest.h>

namespace fs = cobs::fs;

static fs::path base_dir = "data/non_acgt_test";
static fs::path work_dir = base_dir / "work";

class non_acgt_test : public ::testing::Test
{
protected:
  static void SetUpTestSuite() {
    cobs::error_code ec;
    fs::create_directories(work_dir, ec);
  }
  static void TearDownTestSuite() {
    cobs::error_code ec;
    fs::remove_all(work_dir, ec);
  }
};


TEST_F(non_acgt_test, non_acgt_test_main_test) {
    fs::path index_file{base_dir / "index.cobs_classic"};
    cobs::ClassicSearch s(index_file);
    fs::path query_file{base_dir / "test_1.fastq.gz"};
    auto query_out_filepath = work_dir / "query.out";
    std::ofstream query_out_fh(query_out_filepath.string());
    cobs::process_query(s, 0.80000000000000004, 0, "", query_file.string(), query_out_fh);
    query_out_fh.close();
    assert_equals_files_with_paths(
      base_dir / "truth.out",
      query_out_filepath);
}
