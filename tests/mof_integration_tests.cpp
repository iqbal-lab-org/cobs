#include "test_util.hpp"
#include <cobs/util/fs.hpp>
#include <gtest/gtest.h>
#include <cstdlib>

namespace fs = cobs::fs;

static fs::path base_dir = "data/mof_integration_tests";
static fs::path input_dir = base_dir / "subsampled_integration_test";
static fs::path work_dir = base_dir / "work";
static const std::vector<const char *> species{"neisseria_gonorrhoeae__01", "dustbin__01", "chlamydia_pecorum__01"};
static const std::vector<const char *> query_lengths{"100", "200", "500", "1000"};

class mof_integration : public ::testing::Test
{
protected:
  static void SetUpTestSuite() {
    std::string command = std::string("tar -zxvf " + (base_dir/"subsampled_integration_test.tar.gz").string() +
    " --directory ") + base_dir.string();
    std::system(command.c_str());
    cobs::error_code ec;
    fs::create_directories(work_dir, ec);
  }
  static void TearDownTestSuite() {
    cobs::error_code ec;
    fs::remove_all(input_dir, ec);
    fs::remove_all(work_dir, ec);
  }
};

TEST_F(mof_integration, classic_construct_mof_tests) {
  std::string file_type("any");
  cobs::ClassicIndexParameters index_params;
  index_params.num_threads = 1;
  index_params.clobber = true;

  for (const char* bacteria : species) {
    const auto cobs_index_filepath = work_dir / "cobs_indexes" / bacteria / "index.cobs_classic";
    cobs::DocumentList filelist(input_dir / "reordered_assemblies" / bacteria, cobs::StringToFileType(file_type));

    cobs::classic_construct(filelist, cobs_index_filepath,
                            work_dir / "cobs_indexes" / bacteria / "temp", index_params);

    assert_equals_files(cobs_index_filepath, input_dir / "COBS_out" / (std::string(bacteria)+".cobs_classic"));
  }
}


TEST_F(mof_integration, queries__mof_tests) {
  for (const char* bacteria : species) {
    std::vector<std::string> index_files{input_dir / "COBS_out" / (std::string(bacteria)+".cobs_classic")};
    std::vector<std::shared_ptr<cobs::IndexSearchFile> > indices = cobs::get_cobs_indexes_given_files(index_files);
    cobs::ClassicSearch s(indices);

    for (const char* query_length : query_lengths) {
      std::string query_file = input_dir / (std::string(bacteria) + "_queries.query_length_" + query_length + ".fa");
      auto query_out_filepath = work_dir / "query_out" / bacteria / query_length / "query.out";
      fs::create_directories(query_out_filepath.parent_path());
      std::ofstream query_out_fh(query_out_filepath.string());

      cobs::process_query(s, 0.80000000000000004, 0, "", query_file, query_out_fh);

      query_out_fh.close();
      assert_equals_files(
        (input_dir / (std::string(bacteria)+".query_results.query_length_"+query_length+".no_load_complete.txt")).string(),
        query_out_filepath.string());
    }
  }
}