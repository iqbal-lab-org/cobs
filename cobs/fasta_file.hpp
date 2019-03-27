/*******************************************************************************
 * cobs/fasta_file.hpp
 *
 * Copyright (c) 2019 Timo Bingmann
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_FASTA_FILE_HEADER
#define COBS_FASTA_FILE_HEADER

#include <cstring>
#include <string>
#include <vector>

#include <cobs/document.hpp>
#include <cobs/util/file.hpp>
#include <cobs/util/fs.hpp>

#include <tlx/die.hpp>

namespace cobs {

class FastaFile
{
public:
    FastaFile(std::string path) {
        is_.open(path);
        die_unless(is_.good());

        char first = is_.get();
        if (first != '>' && first != ';')
            die("FastaFile: file does not start with > or ;");

        compute_index();
    }

    //! read complete FASTA file for sub-documents
    void compute_index() {
        is_.clear();
        is_.seekg(0);

        std::string line;
        // read first line
        std::getline(is_, line);

        do {
            if (line.size() == 0 || line[0] == ';') {
                // ; comment header
                std::getline(is_, line);
            }
            else if (line[0] == '>') {
                // > document header
                Document doc;
                doc.name = line;
                doc.pos_begin = is_.tellg();
                doc.size = 0;

                while (std::getline(is_, line)) {
                    if (line[0] == '>' || line[0] == ';')
                        break;

                    doc.size += line.size();
                }

                index_.emplace_back(doc);
                // next line is already read
            }
            else if (line[0] == '\r') {
                // skip newline
                std::getline(is_, line);
            }
            else {
                std::cout << "fasta: invalid line " << line << std::endl;
                std::getline(is_, line);
            }
        }
        while (is_.good());
    }

    //! return number of sub-documents
    size_t num_documents() const {
        return index_.size();
    }

    //! return number of terms in a sub-document
    size_t num_terms(size_t doc_index, size_t term_size) const {
        if (doc_index >= index_.size())
            return 0;
        if (index_[doc_index].size < term_size)
            return 0;
        return index_[doc_index].size - term_size + 1;
    }

    template <typename Callback>
    void process_terms(size_t doc_index, size_t term_size, Callback callback) {
        if (doc_index >= index_.size())
            return;

        is_.clear();
        is_.seekg(index_[doc_index].pos_begin);
        die_unless(is_.good());

        std::string data, line;

        while (std::getline(is_, line)) {
            if (line[0] == '>' || line[0] == ';')
                break;

            data += line;
            if (data.empty())
                continue;

            for (size_t i = 0; i + term_size <= data.size(); ++i) {
                // TODO: use std::string_view or similar
                std::string term = data.substr(i, term_size);
                callback(term);
            }
            data.erase(0, data.size() - term_size + 1);
        }
    }

    //! metadata structure for sub-documents
    struct Document {
        std::string name;
        std::istream::pos_type pos_begin;
        size_t size;
    };

    //! index of sub-documents
    std::vector<Document> index_;

private:
    //! file stream
    std::ifstream is_;
};

} // namespace cobs

#endif // !COBS_FASTA_FILE_HEADER

/******************************************************************************/