#pragma once
#include <string>
#include <chrono>
#include <vector>

struct Book {
    std::string title;
    double price = 0.0;
    int stars = 0;
    bool in_stock = false;
};

struct DownloadMetadata {
    std::string url;
    size_t size = 0;
    std::chrono::system_clock::time_point download_time;
    std::string status;
};

struct PageContent {
    std::string url;
    std::string html;
    DownloadMetadata metadata;
};

struct PipelineData {
    std::string url;
    std::string html;
    std::vector<Book> books;
    DownloadMetadata metadata;
    bool success = false;
};

