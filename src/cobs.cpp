/*******************************************************************************
 * src/cobs.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 * Copyright (c) 2018 Timo Bingmann
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/construction/classic_index.hpp>
#include <cobs/construction/compact_index.hpp>
#include <cobs/cortex_file.hpp>
#include <cobs/query/classic_index/mmap_search_file.hpp>
#include <cobs/query/classic_search.hpp>
#include <cobs/query/compact_index/mmap_search_file.hpp>
#include <cobs/query/search.hpp>
#include <cobs/settings.hpp>
#include <cobs/util/calc_signature_size.hpp>
#include <cobs/util/fs.hpp>
#ifdef __linux__
    #include <cobs/query/compact_index/aio_search_file.hpp>
#endif

#include <tlx/cmdline_parser.hpp>
#include <tlx/string.hpp>

#include <cmath>
#include <map>
#include <random>
#include <unordered_map>
#include <unordered_set>


#define VERSION "0.3.1"


/******************************************************************************/

// some often repeated strings
static const char* s_help_file_type =
    "\"list\" to read a file list, or "
    "filter documents by file type (any, text, cortex, fasta, fastq, etc)";

/******************************************************************************/
// Document List and Dump

static void print_document_list(cobs::DocumentList& filelist,
                                uint64_t term_size,
                                std::ostream& os = std::cout) {
    uint64_t min_kmers = uint64_t(-1), max_kmers = 0, total_kmers = 0;

    os << "--- document list (" << filelist.size() << " entries) ---"
       << std::endl;

    for (uint64_t i = 0; i < filelist.size(); ++i) {
        uint64_t num_terms = filelist[i].num_terms(term_size);
        os << "document[" << i << "] size "
           << cobs::fs::file_size(filelist[i].path_)
           << " " << term_size << "-mers " << num_terms
           << " : " << filelist[i].path_
           << " : " << filelist[i].name_
           << std::endl;
        min_kmers = std::min(min_kmers, num_terms);
        max_kmers = std::max(max_kmers, num_terms);
        total_kmers += num_terms;
    }
    os << "--- end of document list (" << filelist.size() << " entries) ---"
       << std::endl;

    os << "documents: " << filelist.size() << std::endl;
    if (filelist.size() != 0) {
        double avg_kmers = static_cast<double>(total_kmers) / filelist.size();
        os << "minimum " << term_size << "-mers: " << min_kmers << std::endl;
        os << "maximum " << term_size << "-mers: " << max_kmers << std::endl;
        os << "average " << term_size << "-mers: "
           << static_cast<uint64_t>(avg_kmers) << std::endl;
        os << "total " << term_size << "-mers: " << total_kmers << std::endl;
    }
}

int doc_list(int argc, char** argv) {
    tlx::CmdlineParser cp;

    std::string path;
    cp.add_param_string(
        "path", path,
        "path to documents to list");

    std::string file_type = "any";
    cp.add_string(
        "file-type", file_type, s_help_file_type);

    unsigned term_size = 31;
    cp.add_unsigned(
        'k', "term-size", term_size,
        "term size (k-mer size), default: 31");

    if (!cp.sort().process(argc, argv))
        return -1;

    cobs::DocumentList filelist(path, cobs::StringToFileType(file_type));
    print_document_list(filelist, term_size);

    return 0;
}

int doc_dump(int argc, char** argv) {
    tlx::CmdlineParser cp;

    std::string path;
    cp.add_param_string(
        "path", path,
        "path to documents to dump");

    unsigned term_size = 31;
    cp.add_unsigned(
        'k', "term-size", term_size,
        "term size (k-mer size), default: 31");

    bool no_canonicalize = false;
    cp.add_flag(
        "no-canonicalize", no_canonicalize,
        "don't canonicalize DNA k-mers, default: false");

    std::string file_type = "any";
    cp.add_string(
        "file-type", file_type, s_help_file_type);

    if (!cp.sort().process(argc, argv))
        return -1;

    cobs::DocumentList filelist(path, cobs::StringToFileType(file_type));

    std::cerr << "Found " << filelist.size() << " documents." << std::endl;

    for (uint64_t i = 0; i < filelist.size(); ++i) {
        std::cerr << "document[" << i << "] : " << filelist[i].path_
                  << " : " << filelist[i].name_ << std::endl;
        std::vector<char> kmer_buffer(term_size);
        filelist[i].process_terms(
            term_size,
            [&](const tlx::string_view& t) {
                if (!no_canonicalize) {
                    bool good = cobs::canonicalize_kmer(
                        t.data(), kmer_buffer.data(), term_size);
                    if (!good)
                        std::cout << "Invalid DNA base pair: " << t
                                  << std::endl;
                    else
                        std::cout << std::string(
                            kmer_buffer.data(), term_size) << '\n';
                }
                else {
                    std::cout << t << '\n';
                }
            });
        std::cout.flush();
        std::cerr << "document[" << i << "] : "
                  << filelist[i].num_terms(term_size) << " terms."
                  << std::endl;
    }

    return 0;
}

/******************************************************************************/
// "Classical" Index Construction

int classic_construct(int argc, char** argv) {
    tlx::CmdlineParser cp;

    cobs::ClassicIndexParameters index_params;

    std::string input;
    cp.add_param_string(
        "input", input, "path to the input directory or file");

    std::string out_file;
    cp.add_param_string(
        "out_file", out_file, "path to the output .cobs_classic index file ");

    std::string file_type = "any";
    cp.add_string(
        "file-type", file_type, s_help_file_type);

    cp.add_bytes(
        'm', "memory", index_params.mem_bytes,
        "memory in bytes to use, default: " +
        tlx::format_iec_units(index_params.mem_bytes));

    cp.add_unsigned(
        'h', "num-hashes", index_params.num_hashes,
        "number of hash functions, default: "
        + std::to_string(index_params.num_hashes));

    cp.add_double(
        'f', "false-positive-rate", index_params.false_positive_rate,
        "false positive rate, default: "
        + std::to_string(index_params.false_positive_rate));

    cp.add_unsigned(
        'k', "term-size", index_params.term_size,
        "term size (k-mer size), default: "
        + std::to_string(index_params.term_size));

    cp.add_bytes(
        's', "sig-size", index_params.signature_size,
        "signature size, default: "
        + std::to_string(index_params.signature_size));

    bool no_canonicalize = false;
    cp.add_flag(
        "no-canonicalize", no_canonicalize,
        "don't canonicalize DNA k-mers, default: false");

    cp.add_flag(
        'C', "clobber", index_params.clobber,
        "erase output directory if it exists");

    cp.add_flag(
        "continue", index_params.continue_,
        "continue in existing output directory");

    cp.add_unsigned(
        'T', "threads", index_params.num_threads,
        "number of threads to use, default: max cores");

    cp.add_flag(
        "keep-temporary", index_params.keep_temporary,
        "keep temporary files during construction");

    std::string tmp_path;
    cp.add_string(
        "tmp-path", tmp_path,
        "directory for intermediate index files, default: out_file + \".tmp\")");

    if (!cp.sort().process(argc, argv))
        return -1;

    cp.print_result(std::cerr);

    // bool to uint8_t
    index_params.canonicalize = !no_canonicalize;

    // read file list
    cobs::DocumentList filelist(input, cobs::StringToFileType(file_type));
    print_document_list(filelist, index_params.term_size);

    cobs::classic_construct(filelist, out_file, tmp_path, index_params);

    return 0;
}

int classic_construct_random(int argc, char** argv) {
    tlx::CmdlineParser cp;

    std::string out_file;
    cp.add_param_string(
        "out-file", out_file, "path to the output file");

    uint64_t signature_size = 2 * 1024 * 1024;
    cp.add_bytes(
        's', "signature-size", signature_size,
        "number of bits of the signatures (vertical size), default: 2 Mi");

    unsigned num_documents = 10000;
    cp.add_unsigned(
        'n', "num-documents", num_documents,
        "number of random documents in index,"
        " default: " + std::to_string(num_documents));

    unsigned document_size = 1000000;
    cp.add_unsigned(
        'm', "document-size", document_size,
        "number of random 31-mers in document,"
        " default: " + std::to_string(document_size));

    unsigned num_hashes = 1;
    cp.add_unsigned(
        'h', "num-hashes", num_hashes,
        "number of hash functions, default: 1");

    unsigned seed = std::random_device { } ();
    cp.add_unsigned("seed", seed, "random seed");

    if (!cp.sort().process(argc, argv))
        return -1;

    cp.print_result(std::cerr);

    std::cerr << "Constructing random index"
              << ", num_documents = " << num_documents
              << ", signature_size = " << signature_size
              << ", result size = "
              << tlx::format_iec_units(num_documents / 8 * signature_size)
              << std::endl;

    cobs::classic_construct_random(
        out_file, signature_size, num_documents, document_size, num_hashes, seed);

    return 0;
}

/******************************************************************************/
// "Compact" Index Construction

int compact_construct(int argc, char** argv) {
    tlx::CmdlineParser cp;

    cobs::CompactIndexParameters index_params;

    std::string input;
    cp.add_param_string(
        "input", input, "path to the input directory or file");

    std::string out_file;
    cp.add_param_string(
        "out_file", out_file, "path to the output .cobs_compact index file");

    std::string file_type = "any";
    cp.add_string(
        "file-type", file_type, s_help_file_type);

    cp.add_bytes(
        'm', "memory", index_params.mem_bytes,
        "memory in bytes to use, default: " +
        tlx::format_iec_units(index_params.mem_bytes));

    cp.add_unsigned(
        'h', "num-hashes", index_params.num_hashes,
        "number of hash functions, default: "
        + std::to_string(index_params.num_hashes));

    cp.add_double(
        'f', "false-positive-rate", index_params.false_positive_rate,
        "false positive rate, default: "
        + std::to_string(index_params.false_positive_rate));

    cp.add_unsigned(
        'k', "term-size", index_params.term_size,
        "term size (k-mer size), default: "
        + std::to_string(index_params.term_size));

    cp.add_unsigned(
        'p', "page-size", index_params.page_size,
        "the page size of the compact the index, "
        "default: sqrt(#documents)");

    bool no_canonicalize = false;
    cp.add_flag(
        "no-canonicalize", no_canonicalize,
        "don't canonicalize DNA k-mers, default: false");

    cp.add_flag(
        'C', "clobber", index_params.clobber,
        "erase output directory if it exists");

    cp.add_flag(
        "continue", index_params.continue_,
        "continue in existing output directory");

    cp.add_unsigned(
        'T', "threads", index_params.num_threads,
        "number of threads to use, default: max cores");

    cp.add_flag(
        "keep-temporary", index_params.keep_temporary,
        "keep temporary files during construction");

    std::string tmp_path;
    cp.add_string(
        "tmp-path", tmp_path,
        "directory for intermediate index files, default: out_file + \".tmp\")");

    if (!cp.sort().process(argc, argv))
        return -1;

    cp.print_result(std::cerr);

    // bool to uint8_t
    index_params.canonicalize = !no_canonicalize;

    // read file list
    cobs::DocumentList filelist(input, cobs::StringToFileType(file_type));
    print_document_list(filelist, index_params.term_size);

    cobs::compact_construct(filelist, out_file, tmp_path, index_params);

    return 0;
}

int compact_construct_combine(int argc, char** argv) {
    tlx::CmdlineParser cp;

    std::string in_dir;
    cp.add_param_string(
        "in-dir", in_dir, "path to the input directory");

    std::string out_file;
    cp.add_param_string(
        "out-file", out_file, "path to the output file");

    unsigned page_size = 8192;
    cp.add_unsigned(
        'p', "page-size", page_size,
        "the page size of the compact the index, "
        "default: 8192");

    if (!cp.sort().process(argc, argv))
        return -1;

    cp.print_result(std::cerr);

    cobs::compact_combine_into_compact(in_dir, out_file, page_size);

    return 0;
}

int classic_combine(int argc, char** argv) {
    tlx::CmdlineParser cp;

    cobs::ClassicIndexParameters index_params;

    std::string in_dir;
    cp.add_param_string(
            "in-dir", in_dir, "path to the input directory");

    std::string out_dir;
    cp.add_param_string(
            "out-dir", out_dir, "path to the output directory");

    std::string out_file;
    cp.add_param_string(
            "out-file", out_file, "path to the output file");

    cp.add_bytes(
            'm', "memory", index_params.mem_bytes,
            "memory in bytes to use, default: " +
            tlx::format_iec_units(index_params.mem_bytes));

    cp.add_unsigned(
            'T', "threads", index_params.num_threads,
            "number of threads to use, default: max cores");

    cp.add_flag(
            "keep-temporary", index_params.keep_temporary,
            "keep temporary files during construction");

    if (!cp.sort().process(argc, argv))
        return -1;

    cp.print_result(std::cerr);

    cobs::fs::path tmp_path(out_dir);
    cobs::fs::path f;
    uint64_t i = 1;

    cobs::fs::copy(in_dir, tmp_path / cobs::pad_index(i));

    while(!cobs::classic_combine(tmp_path / cobs::pad_index(i), tmp_path / cobs::pad_index(i + 1),
                                 f, index_params.mem_bytes, index_params.num_threads,
                                 index_params.keep_temporary)) {
        i++;
    };
    cobs::fs::rename(f, out_file);

    return 0;
}

/******************************************************************************/
int query(int argc, char** argv) {
    tlx::CmdlineParser cp;

    std::vector<std::string> index_files;
    cp.add_stringlist(
        'i', "index", index_files, "path to index file(s)");

    std::string query;
    cp.add_opt_param_string(
        "query", query, "the text sequence to search for");

    std::string query_file;
    cp.add_string(
        'f', "file", query_file, "query (fasta) file to process");

    double threshold = 0.8;
    cp.add_double(
        't', "threshold", threshold,
        "threshold in percentage of terms in query matching, default: 0.8");

    unsigned num_results = 0;
    cp.add_unsigned(
        'l', "limit", num_results,
        "number of results to return, default: all");

    cp.add_flag(
        "load-complete", cobs::gopt_load_complete_index,
        "load complete index into RAM for batch queries");

    cp.add_unsigned(
        'T', "threads", cobs::gopt_threads,
        "number of threads to use, default: max cores");

    std::vector<std::string> index_sizes_str;
    cp.add_stringlist(
      "index-sizes", index_sizes_str, "WARNING: HIDDEN OPTION. USE ONLY IF YOU KNOW WHAT YOU ARE DOING. "
                                      "Precomputed file sizes of the index. Useful if --load-complete is given and "
                                      "indexes are streamed into COBS. This is a hidden option to be used with mof. "
                                      "This also implies COBS classic index, skipping double header reading due to "
                                      "streaming.");

    if (!cp.sort().process(argc, argv))
        return -1;

    std::vector<cobs::fs::path> index_paths;
    for (const std::string &file : index_files) {
        index_paths.push_back(cobs::fs::path(file));
    }
    std::vector<int64_t> index_sizes;
    for (const std::string &index_size_str : index_sizes_str) {
        index_sizes.push_back(std::stoll(index_size_str));
    }
    const bool index_sizes_was_given = index_sizes.size() > 0;
    if (index_sizes_was_given) {
        die_verbose_unequal(index_paths.size(), index_sizes.size(), "If --index-sizes is used, COBS needs a size for each index.");
    }

    std::vector<std::ifstream*> streams;
    std::vector<std::shared_ptr<cobs::IndexSearchFile> > indices;
    if (index_sizes_was_given) {
        for (const cobs::fs::path &index_path : index_paths) {
            auto* stream = new std::ifstream(index_path.string());
            streams.push_back(stream);
        }
        indices = cobs::get_cobs_indexes_given_streams(streams, index_sizes);
    }
    else {
        indices = cobs::get_cobs_indexes_given_files(index_paths);
    }

    cobs::ClassicSearch s(indices);
    cobs::process_query(s, threshold, num_results, query, query_file);

    if (index_sizes_was_given) {
        for (auto* stream : streams) {
            delete stream;
        }
    }

    return 0;
}

/******************************************************************************/
// Miscellaneous Methods

int print_parameters(int argc, char** argv) {
    tlx::CmdlineParser cp;

    unsigned num_hashes = 1;
    cp.add_unsigned(
        'h', "num-hashes", num_hashes,
        "number of hash functions, default: 1");

    double false_positive_rate = 0.3;
    cp.add_double(
        'f', "false-positive-rate", false_positive_rate,
        "false positive rate, default: 0.3");

    uint64_t num_elements = 0;
    cp.add_bytes(
        'n', "num-elements", num_elements,
        "number of elements to be inserted into the index");

    if (!cp.sort().process(argc, argv))
        return -1;

    if (num_elements == 0) {
        double signature_size_ratio =
            cobs::calc_signature_size_ratio(num_hashes, false_positive_rate);
        std::cout << signature_size_ratio << '\n';
    }
    else {
        uint64_t signature_size =
            cobs::calc_signature_size(num_elements, num_hashes, false_positive_rate);
        std::cout << "signature_size = " << signature_size << '\n';
        std::cout << "signature_bytes = " << signature_size / 8
                  << " = " << tlx::format_iec_units(signature_size / 8)
                  << '\n';
    }

    return 0;
}

int print_kmers(int argc, char** argv) {
    tlx::CmdlineParser cp;

    std::string query;
    cp.add_param_string(
        "query", query, "the dna sequence to search for");

    unsigned kmer_size = 31;
    cp.add_unsigned(
        'k', "kmer-size", kmer_size,
        "the size of one kmer, default: 31");

    if (!cp.sort().process(argc, argv))
        return -1;

    std::vector<char> kmer_buffer(kmer_size);
    for (uint64_t i = 0; i < query.size() - kmer_size; i++) {
        bool good = cobs::canonicalize_kmer(
            query.data() + i, kmer_buffer.data(), kmer_size);

        if (!good)
            std::cout << "Invalid DNA base pair: "
                      << std::string(query.data() + i, kmer_size)
                      << std::endl;
        else
            std::cout << std::string(
                kmer_buffer.data(), kmer_size) << '\n';
    }

    return 0;
}

/******************************************************************************/
// Benchmarking

template <bool FalsePositiveDist>
void benchmark_fpr_run(const cobs::fs::path& p,
                       const std::vector<std::string>& queries,
                       const std::vector<std::string>& warmup_queries) {

    cobs::ClassicSearch s(
        std::make_shared<cobs::ClassicIndexMMapSearchFile>(p));

    sync();
    std::ofstream ofs("/proc/sys/vm/drop_caches");
    ofs << "3" << std::endl;
    ofs.close();

    std::vector<cobs::SearchResult> result;
    for (uint64_t i = 0; i < warmup_queries.size(); i++) {
        s.search(warmup_queries[i], result);
    }
    s.timer().reset();

    std::map<uint32_t, uint64_t> counts;

    for (uint64_t i = 0; i < queries.size(); i++) {
        s.search(queries[i], result);

        if (FalsePositiveDist) {
            for (const auto& r : result) {
                counts[r.score]++;
            }
        }
    }

    std::string sse2 = "off";
    std::string aio = "on";

#if __SSE2__
    sse2 = "on";
#endif

#ifdef NO_AIO
    aio = "off";
#endif

    cobs::Timer t = s.timer();
    std::cout << "RESULT"
              << " name=benchmark "
              << " index=" << p.string()
              << " kmer_queries=" << (queries[0].size() - 30)
              << " queries=" << queries.size()
              << " warmup=" << warmup_queries.size()
              << " results=" << result.size()
              << " sse2=" << sse2
              << " aio=" << aio
              << " t_hashes=" << t.get("hashes")
              << " t_io=" << t.get("io")
              << " t_and=" << t.get("and rows")
              << " t_add=" << t.get("add rows")
              << " t_sort=" << t.get("sort results")
              << std::endl;

    for (const auto& c : counts) {
        std::cout << "RESULT"
                  << " name=benchmark_fpr"
                  << " fpr=" << c.first
                  << " dist=" << c.second
                  << std::endl;
    }
}

int benchmark_fpr(int argc, char** argv) {
    tlx::CmdlineParser cp;

    std::string in_file;
    cp.add_param_string(
        "in_file", in_file, "path to the input file");

    unsigned num_kmers = 1000;
    cp.add_unsigned(
        'k', "num-kmers", num_kmers,
        "number of kmers of each query, "
        "default: " + std::to_string(num_kmers));

    unsigned num_queries = 10000;
    cp.add_unsigned(
        'q', "queries", num_queries,
        "number of random queries to run, "
        "default: " + std::to_string(num_queries));

    unsigned num_warmup = 100;
    cp.add_unsigned(
        'w', "warmup", num_warmup,
        "number of random warmup queries to run, "
        "default: " + std::to_string(num_warmup));

    bool fpr_dist = false;
    cp.add_flag(
        'd', "dist", fpr_dist,
        "calculate false positive distribution");

    unsigned seed = std::random_device { } ();
    cp.add_unsigned("seed", seed, "random seed");

    if (!cp.sort().process(argc, argv))
        return -1;

    std::mt19937 rng(seed);

    std::vector<std::string> warmup_queries;
    std::vector<std::string> queries;
    for (uint64_t i = 0; i < num_warmup; i++) {
        warmup_queries.push_back(
            cobs::random_sequence_rng(num_kmers + 30, rng));
    }
    for (uint64_t i = 0; i < num_queries; i++) {
        queries.push_back(
            cobs::random_sequence_rng(num_kmers + 30, rng));
    }

    if (fpr_dist)
        benchmark_fpr_run</* FalsePositiveDist */ true>(
            in_file, queries, warmup_queries);
    else
        benchmark_fpr_run</* FalsePositiveDist */ false>(
            in_file, queries, warmup_queries);

    return 0;
}

/******************************************************************************/

int generate_queries(int argc, char** argv) {
    tlx::CmdlineParser cp;

    std::string path;
    cp.add_param_string(
        "path", path,
        "path to base documents");

    std::string file_type = "any";
    cp.add_string(
        "file-type", file_type, s_help_file_type);

    cp.add_unsigned(
        'T', "threads", cobs::gopt_threads,
        "number of threads to use, default: max cores");

    unsigned term_size = 31;
    cp.add_unsigned(
        'k', "term-size", term_size,
        "term size (k-mer size), default: 31");

    unsigned num_positive = 0;
    cp.add_unsigned(
        'p', "positive", num_positive,
        "pick this number of existing positive queries, default: 0");

    unsigned num_negative = 0;
    cp.add_unsigned(
        'n', "negative", num_negative,
        "construct this number of random non-existing negative queries, "
        "default: 0");

    bool true_negatives = false;
    cp.add_flag(
        'N', "true-negative", true_negatives,
        "check that negative queries actually are not in the documents (slow)");

    unsigned fixed_size = 0;
    cp.add_unsigned(
        's', "size", fixed_size,
        "extend positive terms with random data to this size");

    unsigned seed = std::random_device { } ();
    cp.add_unsigned(
        'S', "seed", seed,
        "random seed");

    std::string out_file;
    cp.add_string(
        'o', "out-file", out_file,
        "output file path");

    if (!cp.sort().process(argc, argv))
        return -1;

    cobs::DocumentList filelist(path, cobs::StringToFileType(file_type));

    std::vector<uint64_t> terms_prefixsum;
    terms_prefixsum.reserve(filelist.size());
    uint64_t total_terms = 0;
    for (uint64_t i = 0; i < filelist.size(); ++i) {
        terms_prefixsum.push_back(total_terms);
        total_terms += filelist[i].num_terms(term_size);
    }

    LOG1 << "Given " << filelist.size() << " documents containing "
         << total_terms << " " << term_size << "-gram terms";

    if (fixed_size < term_size)
        fixed_size = term_size;

    std::default_random_engine rng(seed);

    struct Query {
        // term string
        std::string term;
        // document index
        uint64_t doc_index;
        // term index inside document
        uint64_t term_index;
    };

    // select random term ids for positive queries
    std::vector<uint64_t> positive_indices;
    std::vector<Query> positives;
    {
        die_unless(total_terms >= num_positive);

        std::unordered_set<uint64_t> positive_set;
        while (positive_set.size() < num_positive) {
            positive_set.insert(rng() % total_terms);
        }

        positive_indices.reserve(num_positive);
        for (auto it = positive_set.begin(); it != positive_set.end(); ++it) {
            positive_indices.push_back(*it);
        }
        std::sort(positive_indices.begin(), positive_indices.end());

        positives.resize(positive_indices.size());
    }

    // generate random negative queries
    std::vector<std::string> negatives;
    std::mutex negatives_mutex;
    std::unordered_map<std::string, std::vector<uint64_t> > negative_terms;
    std::unordered_set<uint64_t> negative_hashes;
    for (uint64_t t = 0; t < 1.5 * num_negative; ++t) {
        std::string neg = cobs::random_sequence_rng(fixed_size, rng);
        negatives.push_back(neg);

        // insert and hash all terms in the query
        for (uint64_t i = 0; i + term_size <= neg.size(); ++i) {
            std::string term = neg.substr(i, term_size);
            negative_terms[term].push_back(t);
            negative_hashes.insert(tlx::hash_djb2(term));
        }
    }

    // run over all documents and fetch positive queries
    cobs::parallel_for(
        0, filelist.size(), cobs::gopt_threads,
        [&](uint64_t d) {
            uint64_t index = terms_prefixsum[d];
            // find starting iterator for positive_indices in this file
            uint64_t pos_index = std::lower_bound(
                positive_indices.begin(), positive_indices.end(), index)
                               - positive_indices.begin();
            uint64_t next_index =
                pos_index < positive_indices.size() ?
                positive_indices[pos_index] : uint64_t(-1);
            if (next_index == uint64_t(-1) && !true_negatives)
                return;

            filelist[d].process_terms(
                term_size,
                [&](const tlx::string_view& term) {
                    if (index == next_index) {
                        // store positive term
                        Query& q = positives[pos_index];
                        q.term = term.to_string();
                        q.doc_index = d;
                        q.term_index = index - terms_prefixsum[d];

                        // extend positive term to term_size by padding
                        if (q.term.size() < fixed_size) {
                            uint64_t padding = fixed_size - q.term.size();
                            uint64_t front_padding = rng() % padding;
                            uint64_t back_padding = padding - front_padding;

                            q.term =
                                cobs::random_sequence_rng(front_padding, rng) +
                                q.term +
                                cobs::random_sequence_rng(back_padding, rng);
                        }

                        ++pos_index;
                        next_index = pos_index < positive_indices.size() ?
                                     positive_indices[pos_index] : uint64_t(-1);
                    }
                    index++;

                    if (true_negatives) {
                        // lookup hash of term in read-only hash table first
                        std::string t = term.to_string();
                        if (negative_hashes.count(tlx::hash_djb2(t)) != 0) {
                            std::unique_lock<std::mutex> lock(negatives_mutex);
                            auto it = negative_terms.find(t);
                            if (it != negative_terms.end()) {
                                // clear negative query with term found
                                LOG1 << "remove false negative: " << t;
                                for (const uint64_t& i : it->second)
                                    negatives[i].clear();
                                negative_terms.erase(it);
                            }
                        }
                    }
                });
        });

    // check if there are enough negatives queries left
    if (negatives.size() < num_negative) {
        die("Not enough true negatives left, you were unlucky, try again.");
    }

    // collect everything and return them in random order
    std::vector<Query> queries = std::move(positives);
    queries.reserve(num_positive + num_negative);

    uint64_t ni = 0;
    for (auto it = negatives.begin(); ni < num_negative; ++it) {
        if (it->empty())
            continue;
        Query q;
        q.term = std::move(*it);
        q.doc_index = uint64_t(-1);
        queries.emplace_back(q);
        ++ni;
    }

    std::shuffle(queries.begin(), queries.end(), rng);

    uint64_t negative_count = 0;

    auto write_output =
        [&](std::ostream& os) {
            for (const Query& q : queries) {
                if (q.doc_index != uint64_t(-1))
                    os << ">doc:" << q.doc_index << ":term:" << q.term_index
                       << ":" << filelist[q.doc_index].name_ << '\n';
                else
                    os << ">negative" << negative_count++ << '\n';
                os << q.term << '\n';
            }
        };

    if (out_file.empty()) {
        write_output(std::cout);
    }
    else {
        std::ofstream os(out_file.c_str());
        write_output(os);
    }

    return 0;
}

/******************************************************************************/

int version(int argc, char** argv) {
  std::cout << "COBS version " << VERSION << std::endl;
  return 0;
}

/******************************************************************************/

struct SubTool {
    const char* name;
    int (* func)(int argc, char* argv[]);
    bool shortline;
    const char* description;
};

struct SubTool subtools[] = {
    {
        "doc-list", &doc_list, true,
        "read a list of documents and print the list"
    },
    {
        "doc-dump", &doc_dump, true,
        "read a list of documents and dump their contents"
    },
    {
        "classic-construct", &classic_construct, true,
        "constructs a classic index from the documents in <in_dir>"
    },
    {
        "classic-construct-random", &classic_construct_random, true,
        "constructs a classic index with random content"
    },
    {
        "compact-construct", &compact_construct, true,
        "creates the folders used for further construction"
    },
    {
        "compact-construct-combine", &compact_construct_combine, true,
        "combines the classic indices in <in_dir> to form a compact index"
    },
    {
            "classic-combine", &classic_combine, true,
            "combines the classic indices in <in_dir>"
    },
    {
        "query", &query, true,
        "query an index"
    },
    {
        "print-parameters", &print_parameters, true,
        "calculates index parameters"
    },
    {
        "print-kmers", &print_kmers, true,
        "print all canonical kmers from <query>"
    },
    {
        "benchmark-fpr", &benchmark_fpr, true,
        "run benchmark and false positive measurement"
    },
    {
        "generate-queries", &generate_queries, true,
        "select queries randomly from documents"
    },
    {
      "version", &version, true, "prints version and exit"
    },
    { nullptr, nullptr, true, nullptr }
};

int main_usage(const char* arg0) {
    std::cout << "(Co)mpact (B)it-Sliced (S)ignature Index for Genome Search (version " << VERSION << ")"
              << std::endl << std::endl;
    std::cout << "Usage: " << arg0 << " <subtool> ..." << std::endl << std::endl
              << "Available subtools: " << std::endl;

    int shortlen = 0;

    for (uint64_t i = 0; subtools[i].name; ++i)
    {
        if (!subtools[i].shortline) continue;
        shortlen = std::max(shortlen, static_cast<int>(strlen(subtools[i].name)));
    }

    for (uint64_t i = 0; subtools[i].name; ++i)
    {
        if (subtools[i].shortline) continue;
        std::cout << "  " << subtools[i].name << std::endl;
        tlx::CmdlineParser::output_wrap(
            std::cout, subtools[i].description, 80, 6, 6);
        std::cout << std::endl;
    }

    for (uint64_t i = 0; subtools[i].name; ++i)
    {
        if (!subtools[i].shortline) continue;
        std::cout << "  " << std::left << std::setw(shortlen + 2)
                  << subtools[i].name << subtools[i].description << std::endl;
    }
    std::cout << std::endl;

    std::cout << "See https://panthema.net/cobs for more information on COBS."
              << std::endl << std::endl;

    return 0;
}

int main(int argc, char** argv) {
    char progsub[256];

    tlx::set_logger_to_stderr();

    if (argc < 2)
        return main_usage(argv[0]);

    for (uint64_t i = 0; subtools[i].name; ++i)
    {
        if (strcmp(subtools[i].name, argv[1]) == 0)
        {
            // replace argv[1] with call string of subtool.
            snprintf(progsub, sizeof(progsub), "%s %s", argv[0], argv[1]);
            argv[1] = progsub;
            try {
                return subtools[i].func(argc - 1, argv + 1);
            }
            catch (std::exception& e) {
                std::cerr << "EXCEPTION: " << e.what() << std::endl;
                return -1;
            }
        }
    }

    std::cout << "Unknown subtool '" << argv[1] << "'" << std::endl;

    return main_usage(argv[0]);
}

/******************************************************************************/
