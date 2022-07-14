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
  const std::vector<std::string> &index_files
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
    std::ifstream qf(query_file);
    std::string line, query, comment;

    while (std::getline(qf, line)) {
      if (line.empty())
        continue;

      if (line[0] == '>' || line[0] == ';') {
        if (!query.empty()) {
          // perform query
          s.search(query, result, threshold, num_results);
          output_stream << comment << '\t' << result.size() << '\n';

          for (const auto &res: result) {
            output_stream << res.doc_name << '\t' << res.score << '\n';
          }
        }

        // clear and copy query comment
        line[0] = '*';
        query.clear();
        comment = line;
        continue;
      } else {
        query += line;
      }
    }

    if (!query.empty()) {
      // perform query
      s.search(query, result, threshold, num_results);
      output_stream << comment << '\t' << result.size() << '\n';

      for (const auto &res: result) {
        output_stream << res.doc_name << '\t' << res.score << '\n';
      }
    }
  } else {
    die("Pass a verbatim query or a query file.");
  }

  s.timer().print("search");
}

} // namespace cobs

#endif // !COBS_QUERY_SEARCH_HEADER

/******************************************************************************/
