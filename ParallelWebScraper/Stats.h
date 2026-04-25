#pragma once
#include <string>
#include "Models.h"
#include <tbb/concurrent_vector.h>
#include <tbb/concurrent_set.h>

void calculate_and_save_statistics(
    const tbb::concurrent_vector<PageContent>& page_contents,
    const tbb::concurrent_set<std::string>& visited_urls,
    const tbb::concurrent_vector<Book>& all_books,
    const std::string& output_filename
);

