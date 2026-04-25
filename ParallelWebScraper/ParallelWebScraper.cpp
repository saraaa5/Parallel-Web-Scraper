// ParallelWebScraper.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include "Downloader.h"
#include "GumboParser.h"
#include "Stats.h"
#include <tbb/parallel_pipeline.h>
#include <tbb/parallel_for.h>
#include <tbb/concurrent_vector.h>
#include <tbb/concurrent_set.h>
#include <tbb/blocked_range.h>
#include <tbb/global_control.h>
#include <iostream>
#include <fstream>
#include <atomic>
#include <set>

using namespace std;
using namespace tbb;

// ucitava urlove iz argumenata komandne linije
vector<string> input_urls(int argc, char* argv[]) {
    set<string> unique_urls;   // automatski odbacuje duplikate
    vector<string> urls;

    for (int i = 1; i < argc; i++) {
        string arg = argv[i];

        if (arg.find("--") == 0) continue;

        if (arg.find("http") == 0 || arg.find("www.") == 0 || arg.find(".html") != string::npos) {
            if (unique_urls.insert(arg).second) { // ubacuje samo ako nije već tu
                urls.push_back(arg);
            }
            else {
                cout << "skipping duplicate url: " << arg << endl;
            }
        }
        else {
            cout << "skipping invalid url: " << arg << endl;
        }
    }

    if (urls.empty()) {
        cerr << "error: no valid urls provided.\n";
        exit(1);
    }

    return urls;
}


// Proveri da li koristimo pipeline iz argumenata komandne linije
bool should_use_pipeline(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "--pipeline") {
            return true;
        }
        if (arg == "--parallel-for") {
            return false;
        }
    }
    // Podrazumevano: koristi pipeline
    return true;
}

// Implementacija sa parallel_pipeline
void process_with_pipeline(const vector<string>& urls,
    concurrent_vector<PageContent>& page_contents,
    concurrent_vector<Book>& all_books) {

    atomic<int> processed_count(0);
    atomic<int> success_count(0);
    const int total_urls = urls.size();

    cout << "\nPARALLEL PIPELINE PROCESSING\n" << endl;
    cout << "processing " << total_urls << " urls using 3-stage pipeline" << endl;
  
    auto pipeline_start = chrono::high_resolution_clock::now();

    // Faza 1: Generisanje urlova
    auto url_source = [&urls](tbb::flow_control& fc) -> PipelineData* {
        static size_t index = 0;

        if (index >= urls.size()) {
            fc.stop();
            return nullptr;
        }

        PipelineData* data = new PipelineData();
        data->url = urls[index++];
        return data;
        };

    // Faza 2: Preuzimanje i parsiranje
    auto download_parse_stage = [](PipelineData* data) -> PipelineData* {
        try {
            auto start_time = chrono::system_clock::now();
            data->html = download_page(data->url);
            auto end_time = chrono::system_clock::now();

            data->metadata.url = data->url;
            data->metadata.size = data->html.size();
            data->metadata.download_time = end_time;
            data->metadata.status = "success";

            data->books = parse_books_gumbo(data->html);
            data->success = true;

            cout << data->url
                << " (" << data->html.size() << " chars, "
                << data->books.size() << " books)" << endl;

        }
        catch (const exception& e) {
            data->metadata.url = data->url;
            data->metadata.size = 0;
            data->metadata.download_time = chrono::system_clock::now();
            data->metadata.status = "failed: " + string(e.what());
            data->success = false;
            data->books.clear();

            cerr << "error " << data->url << ": " << e.what() << endl;
        }

        return data;
        };

    // Faza 3: Cuvanje rezultata
    auto save_results_stage = [&page_contents, &all_books, &processed_count, &success_count, total_urls](PipelineData* data) {
        // Cuvanje metapodataka
        PageContent content;
        content.url = data->url;
        content.html = data->html;
        content.metadata = data->metadata;
        page_contents.push_back(content);

        // cuvanje knjiga
        if (data->success) {
            for (auto& book : data->books) {
                all_books.push_back(book);
            }
            success_count++;
        }
        delete data;
        };

    // Pokretanje pipeline-a sa 3 faze
    parallel_pipeline(
        max(4, (int)urls.size()), 
        make_filter<void, PipelineData*>(filter_mode::serial_in_order, url_source) &
        make_filter<PipelineData*, PipelineData*>(filter_mode::parallel, download_parse_stage) &
        make_filter<PipelineData*, void>(filter_mode::serial_in_order, save_results_stage)
    );

    auto pipeline_end = chrono::high_resolution_clock::now();
    auto pipeline_duration = chrono::duration_cast<chrono::milliseconds>(pipeline_end - pipeline_start);

    cout << "\nPIPELINE COMPLETE" << endl;

}

void process_with_parallel_for(const vector<string>& urls,
    concurrent_vector<PageContent>& page_contents,
    concurrent_vector<Book>& all_books) {

    cout << "\nPARALLEL FOR PROCESSING\n" << endl;
    auto start_time = chrono::high_resolution_clock::now();

    parallel_for(blocked_range<size_t>(0, urls.size()),
        [&](const blocked_range<size_t>& range) {
            for (size_t i = range.begin(); i != range.end(); ++i) {
                try {
                    auto download_start = chrono::system_clock::now();
                    string html = download_page(urls[i]);
                    auto download_end = chrono::system_clock::now();

                    DownloadMetadata metadata;
                    metadata.url = urls[i];
                    metadata.size = html.size();
                    metadata.download_time = download_end;
                    metadata.status = "success";

                    PageContent content;
                    content.url = urls[i];
                    content.html = html;
                    content.metadata = metadata;

                    page_contents.push_back(content);

                    auto books = parse_books_gumbo(html);
                    for (auto& b : books) {
                        all_books.push_back(b);
                    }

                    cout << urls[i]
                        << " (" << html.size() << " chars, "
                        << books.size() << " books)" << endl;
                }
                catch (const exception& e) {
                    DownloadMetadata metadata;
                    metadata.url = urls[i];
                    metadata.size = 0;
                    metadata.download_time = chrono::system_clock::now();
                    metadata.status = "failed: " + string(e.what());

                    PageContent content;
                    content.url = urls[i];
                    content.html = "";
                    content.metadata = metadata;

                    page_contents.push_back(content);
                    cerr << "error" << urls[i] << ": " << e.what() << endl;
                }
            }
        });

    auto end_time = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);

    cout << "\nPARALLEL FOR COMPLETE" << endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "using: " << argv[0] << " <URL1> <URL2> ... <URLn>\n";
        cerr << "       " << argv[0] << " --pipeline (use pipeline processing)\n";
        cerr << "       " << argv[0] << " --parallel-for (use parallel_for processing)\n";
        return 1;
    }
    //global_control control(global_control::max_allowed_parallelism, 4);
    bool use_pipeline = should_use_pipeline(argc, argv);

    vector<string> urls = input_urls(argc, argv);

    if (urls.empty()) {
        cerr << "Error: No URLs provided. Please provide at least one URL." << endl;
        return 1;
    }

    cout << "\nWEB SCRAPER STARTED" << endl;

    concurrent_vector<PageContent> page_contents;
    concurrent_vector<Book> all_books;
    concurrent_set<string> visited_urls;

    // Inicijalizacija cURL
    curl_global_initialize();

    auto total_start = chrono::high_resolution_clock::now();

    // Obrada URL-ova odabranom metodom
    if (use_pipeline) {
        process_with_pipeline(urls, page_contents, all_books);
    }
    else {
        process_with_parallel_for(urls, page_contents, all_books);
    }

    auto total_end = chrono::high_resolution_clock::now();
    auto total_duration = chrono::duration_cast<chrono::milliseconds>(total_end - total_start);

    // Cleanup cURL
    curl_global_cleanup_wrapper();

    // Popuni visited_urls za statistiku
    for (const auto& content : page_contents) {
        visited_urls.insert(content.url);
    }

    cout << "\nFINAL RESULTS" << endl;
    cout << "execution time: " << total_duration.count() << " ms" << endl;
    cout << "pages processed: " << page_contents.size() << endl;
    cout << "books found: " << all_books.size() << endl;
    cout << "processing method: " << (use_pipeline ? "Pipeline" : "Parallel For") << endl;

    // Statistika uspeha
    int success_count = 0;
    for (const auto& content : page_contents) {
        if (content.metadata.status == "success") {
            success_count++;
        }
    }

    cout << "success rate: " << success_count << "/" << page_contents.size()
        << " (" << (success_count * 100.0 / page_contents.size()) << "%)" << endl;

    string results_file = use_pipeline ? "results_pipeline.txt" : "results_parallel_for.txt";
    calculate_and_save_statistics(page_contents, visited_urls, all_books, results_file);

    return 0;
}