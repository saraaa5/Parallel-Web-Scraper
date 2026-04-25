#include "Downloader.h"
#include <curl/curl.h>
#include <stdexcept>
#include <thread>
#include <chrono>
#include <iostream>

using namespace std;

namespace {
    size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        ((string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }
}
//ada libcurl preuzme deo podataka sa urla, poziva ovu funkciju da bi obradio te podatke

void curl_global_initialize() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

void curl_global_cleanup_wrapper() {
    curl_global_cleanup();
}

string download_page(const string& url, int retries, long timeout) {
    CURL* curl;
    CURLcode res;
    string readBuffer;

    curl = curl_easy_init();
    if (!curl) throw runtime_error("error: curl_easy_init() failed");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    for (int attempt = 1; attempt <= retries; attempt++) {
        res = curl_easy_perform(curl);
        if (res == CURLE_OK) break;
        cerr << "try num " << attempt << " did not make it: " << url
            << " (" << curl_easy_strerror(res) << ")\n";
        if (attempt == retries) {
            curl_easy_cleanup(curl);
            throw runtime_error("error: cannot download the page " + url);
        }
        this_thread::sleep_for(chrono::milliseconds(500));
    }

    curl_easy_cleanup(curl);
    return readBuffer;
}