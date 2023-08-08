/*******************************************************************************
 * cobs/query/search.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 * Copyright (c) 2018 Timo Bingmann
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_QUERY_SEARCH_HEADER
#define COBS_QUERY_SEARCH_HEADER

#include <cobs/util/timer.hpp>
#include <cobs/util/file.hpp>
#include <iostream>
#include <fstream>
#include <tlx/die.hpp>
#include <cobs/file/compact_index_header.hpp>
#include <cobs/file/classic_index_header.hpp>
#include <cobs/query/compact_index/mmap_search_file.hpp>
#include <cobs/query/classic_index/mmap_search_file.hpp>
#include <zlib.h>
#include <kseq.h>
KSEQ_INIT(gzFile, gzread)


namespace cobs {

struct SearchResult {
    //! string reference to document name
    const char* doc_name;
    //! score (number of matched k-mers)
    uint32_t score;

    SearchResult() = default;

    SearchResult(const char* doc_name, uint32_t score)
        : doc_name(doc_name), score(score) { }
};

class Search
{
public:
    virtual ~Search() = default;

    //! Returns timer_
    Timer& timer() { return timer_; }
    //! Returns timer_
    const Timer& timer() const { return timer_; }

    virtual void search(
        const std::string& query,
        std::vector<SearchResult>& result,
        double threshold = 0.0, uint64_t num_results = 0) = 0;

public:
    //! timer of different query phases
    Timer timer_;
};

static inline std::vector<std::shared_ptr<cobs::IndexSearchFile> > get_cobs_indexes_given_files (
  const std::vector<fs::path> &index_files
  ) {
  std::vector<std::shared_ptr<cobs::IndexSearchFile> > indices;

  for (auto& path : index_files)
  {
    if (cobs::file_has_header<cobs::ClassicIndexHeader>(path)) {
      indices.push_back(
        std::make_shared<cobs::ClassicIndexMMapSearchFile>(path));
    }
    else if (cobs::file_has_header<cobs::CompactIndexHeader>(path)) {
      indices.push_back(
        std::make_shared<cobs::CompactIndexMMapSearchFile>(path));
    }
    else
      die("Could not open index path \"" << path << "\"");
  }

  return indices;
}

static inline std::vector<std::shared_ptr<cobs::IndexSearchFile> > get_cobs_indexes_given_streams (
    const std::vector<std::ifstream*> &streams, const std::vector<int64_t> &index_sizes) {

    std::vector<std::shared_ptr<cobs::IndexSearchFile> > indices;
    for (size_t i=0; i<streams.size(); i++)
    {
        std::ifstream *stream = streams[i];
        int64_t index_file_size = index_sizes[i];
        indices.push_back(
          std::make_shared<cobs::ClassicIndexMMapSearchFile>(*stream, index_file_size));
    }

    return indices;
}

static inline void process_query(
  cobs::Search &s, double threshold, unsigned num_results,
  const std::string &query_line, const std::string &query_file,
  std::ostream &output_stream = std::cout) {
  std::vector<cobs::SearchResult> result;

  if (!query_line.empty()) {
    s.search(query_line, result, threshold, num_results);

    for (const auto &res: result) {
      output_stream << res.doc_name << '\t' << res.score << '\n';
    }
  } else if (!query_file.empty()) {
    gzFile fp = gzopen(query_file.c_str(), "r");
    if (!fp)
      die("Could not open query file: " + query_file);

    kseq_t *seq = kseq_init(fp);
    while (kseq_read(seq) >= 0) {
      std::string comment(seq->name.s ? seq->name.s : "");
      std::string query(seq->seq.s ? seq->seq.s : "");

      // perform query
      s.search(query, result, threshold, num_results);
      output_stream << "*" << comment << '\t' << result.size() << '\n';

      for (const auto &res: result) {
        output_stream << res.doc_name << '\t' << res.score << '\n';
      }
    }
    kseq_destroy(seq);
    gzclose(fp);
  } else {
    die("Pass a verbatim query or a query file.");
  }

  s.timer().print("search");
}

} // namespace cobs

#endif // !COBS_QUERY_SEARCH_HEADER

/******************************************************************************/
