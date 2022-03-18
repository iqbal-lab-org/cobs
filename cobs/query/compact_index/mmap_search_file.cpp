/*******************************************************************************
 * cobs/query/compact_index/mmap_search_file.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/query/compact_index/mmap_search_file.hpp>
#include <cobs/util/query.hpp>

#include <tlx/logger.hpp>
#include <tlx/math/div_ceil.hpp>

namespace cobs {

CompactIndexMMapSearchFile::CompactIndexMMapSearchFile(const fs::path& path)
    : CompactIndexSearchFile(path)
{
    data_.resize(header_.parameters_.size());
    handle_ = initialize_mmap(path);
    data_[0] = handle_.data + stream_pos_.curr_pos;
    for (uint64_t i = 1; i < header_.parameters_.size(); i++) {
        data_[i] =
            data_[i - 1]
            + header_.page_size_ * header_.parameters_[i - 1].signature_size;
    }
}

CompactIndexMMapSearchFile::~CompactIndexMMapSearchFile() {
    destroy_mmap(handle_);
}

void CompactIndexMMapSearchFile::read_from_disk(
    const std::vector<uint64_t>& hashes, uint8_t* rows,
    uint64_t begin, uint64_t size, uint64_t buffer_size)
{
    uint64_t page_size = header_.page_size_;

    die_unless(begin + size <= row_size());
    die_unless(begin % page_size == 0);
    uint64_t begin_page = begin / page_size;
    uint64_t end_page = tlx::div_ceil(begin + size, page_size);
    die_unless(end_page <= header_.parameters_.size());

    LOG0 << "mmap::read_from_disk()"
         << " page_size=" << page_size
         << " hashes.size=" << hashes.size()
         << " begin=" << begin
         << " size=" << size
         << " buffer_size=" << buffer_size
         << " begin_page=" << begin_page
         << " end_page=" << end_page;

    for (uint64_t i = 0; i < hashes.size(); i++) {
        uint64_t j = 0;
        for (uint64_t p = begin_page; p < end_page; ++p, ++j) {
            uint64_t hash = hashes[i] % header_.parameters_[p].signature_size;
            uint8_t* data_8 = data_[p] + hash * page_size;
            uint8_t* rows_8 =
                reinterpret_cast<uint8_t*>(rows) + i * buffer_size + j * page_size;
            // die_unless(rows_8 + page_size <= rows + size * hashes.size());
            // std::memcpy(rows_8, data_8, page_size);
            std::copy(data_8, data_8 + page_size, rows_8);
        }
    }
}

} // namespace cobs

/******************************************************************************/
