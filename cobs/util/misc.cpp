/*******************************************************************************
 * cobs/util/misc.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/util/misc.hpp>

#include <unistd.h>
#include <sys/types.h>
#include <sys/sysctl.h>

#include <tlx/die.hpp>


// portable way to get phys_pages from https://stackoverflow.com/a/30512156/5264075
#define PHYS_PAGES get_phys_pages()

uint64_t get_phys_pages () {
  static uint64_t phys_pages;
  if (phys_pages == 0) {
#ifdef __APPLE__
    uint64_t mem;
    uint64_t len = sizeof(mem);
    sysctlbyname("hw.memsize", &mem, &len, NULL, 0);
    phys_pages = mem/sysconf(_SC_PAGE_SIZE);
#endif
#ifdef __linux__
    phys_pages = sysconf(_SC_PHYS_PAGES);
#endif
  }
  return phys_pages;
}

namespace cobs {

uint64_t get_page_size() {
    int page_size = getpagesize();
    die_unless(page_size > 0);
    die_unless(page_size == 4096);     // todo check for experiments
    return (uint64_t)page_size;
}

uint64_t get_memory_size() {
    return PHYS_PAGES * sysconf(_SC_PAGE_SIZE);
}

uint64_t get_memory_size(uint64_t percentage) {
    return get_memory_size() * percentage / 100;
}

std::string random_sequence(uint64_t size, uint64_t seed) {
    std::default_random_engine rng(seed);
    return random_sequence_rng(size, rng);
}

} // namespace cobs

/******************************************************************************/
