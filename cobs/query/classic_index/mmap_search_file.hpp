/*******************************************************************************
 * cobs/query/classic_index/mmap_search_file.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_QUERY_CLASSIC_INDEX_MMAP_SEARCH_FILE_HEADER
#define COBS_QUERY_CLASSIC_INDEX_MMAP_SEARCH_FILE_HEADER

#include <cobs/query/classic_index/search_file.hpp>

namespace cobs {

class ClassicIndexMMapSearchFile : public ClassicIndexSearchFile
{
private:
    MMapHandle handle_;
    uint8_t* data_;

protected:
    void read_from_disk(const std::vector<uint64_t>& hashes, uint8_t* rows,
                        uint64_t begin, uint64_t size, uint64_t buffer_size) override;

public:
    explicit ClassicIndexMMapSearchFile(const fs::path& path);
    ~ClassicIndexMMapSearchFile();
};

} // namespace cobs

#endif // !COBS_QUERY_CLASSIC_INDEX_MMAP_SEARCH_FILE_HEADER

/******************************************************************************/
