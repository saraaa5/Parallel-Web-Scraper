#pragma once
#include <string>

void curl_global_initialize();
void curl_global_cleanup_wrapper();
std::string download_page(const std::string& url, int retries = 3, long timeout = 5);