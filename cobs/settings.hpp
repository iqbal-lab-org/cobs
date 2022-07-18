/*******************************************************************************
 * cobs/settings.hpp
 *
 * Copyright (c) 2019 Timo Bingmann
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_SETTINGS_HEADER
#define COBS_SETTINGS_HEADER

#include <cstdlib>
#include <stdint.h>


namespace cobs {

//! run COBS using parallel threads, default: all cores
extern unsigned gopt_threads;

//! whether to load the complete index to RAM for queries.
extern bool gopt_load_complete_index;

//! whether to disable FastA/FastQ cache files globally.
extern bool gopt_disable_cache;

} // namespace cobs

#endif // !COBS_SETTINGS_HEADER

/******************************************************************************/
