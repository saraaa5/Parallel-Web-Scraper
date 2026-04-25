#pragma once

#pragma once
#include <string>
#include <vector>
#include <tbb/concurrent_vector.h>
#include "GumboParser.h"


    WebsiteIndexer(const std::string& base_url);

    tbb::concurrent_vector<std::string> discover_book_pages(int max_pages = 50);

    std::string base_url_;

    tbb::concurrent_vector<std::string> get_all_categories();
    tbb::concurrent_vector<std::string> get_category_book_urls(const std::string& category_url);
    std::string make_absolute_url(const std::string& relative_url);
    int extract_last_page_number(const std::string& html);
