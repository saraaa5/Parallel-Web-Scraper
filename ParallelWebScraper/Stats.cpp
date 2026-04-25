#include "Stats.h"
#include <fstream>
#include <algorithm>
#include <limits>
#include <iostream>
#include <chrono>
using namespace tbb;
using namespace std;

void calculate_and_save_statistics(const concurrent_vector<PageContent>& page_contents,
    const concurrent_set<string>& visited_urls,
    const concurrent_vector<Book>& all_books,
    const string& output_filename) {
    ofstream out_file(output_filename);

    if (!out_file.is_open()) {
        cerr << "error: cannot open out file " << output_filename << endl;
        return;
    }

    out_file << "OVERALL STATISTICS" << endl;
    out_file << "num of downloaded pages: " << page_contents.size() << endl;
    out_file << "num of unicate urls: " << visited_urls.size() << endl;


    if (!page_contents.empty()) {
        auto first_time = page_contents[0].metadata.download_time;
        auto last_time = page_contents[0].metadata.download_time;

        for (const auto& content : page_contents) {
            if (content.metadata.download_time < first_time)
                first_time = content.metadata.download_time;
            if (content.metadata.download_time > last_time)
                last_time = content.metadata.download_time;
        }

        auto total_duration = chrono::duration_cast<chrono::milliseconds>(last_time - first_time);
        double seconds = total_duration.count() / 1000.0;
        double pages_per_second = seconds > 0.0 ?
            page_contents.size() / seconds : page_contents.size();

        out_file << "total download time: " << total_duration.count() << " ms" << endl;
        out_file << "permeability: " << pages_per_second << " pages per second" << endl;
    }

    out_file << "\n ANALYTICS OF PAGE CONTENT" << endl;

    if (all_books.empty()) {
        out_file << "no book found" << endl;
        out_file.close();
        return;
    }

    int five_star_books = count_if(all_books.begin(), all_books.end(),
        [](const Book& book) { return book.stars == 5; });
    out_file << "1. num of books with 5 stars: " << five_star_books << endl;

    double total_price = 0.0;
    int books_with_price = 0;

    for (const auto& book : all_books) {
        if (book.price > 0) {
            total_price += book.price;
            books_with_price++;
        }
    }

    double average_price = books_with_price > 0 ? total_price / books_with_price : 0.0;
    out_file << "2. average book price: " << average_price << " GBP" << endl;

    double min_price = numeric_limits<double>::max();
    double max_price = numeric_limits<double>::lowest();

    for (const auto& book : all_books) {
        if (book.price > 0) {
            min_price = min(min_price, book.price);
            max_price = max(max_price, book.price);
        }
    }

    out_file << "3. the cheapest book: " << (min_price != numeric_limits<double>::max() ? to_string(min_price) : "N/A") << " GBP" << endl;
    out_file << " and the most expensive book: " << (max_price != numeric_limits<double>::lowest() ? to_string(max_price) : "N/A") << " GBP" << endl;

    int available_books = count_if(all_books.begin(), all_books.end(), [](const Book& book) { return book.in_stock; });
    out_file << "4. num of books in stock: " << available_books << endl;

    int books_starting_with_T = count_if(all_books.begin(), all_books.end(),
        [](const Book& book) {
            if (!book.title.empty()) {
                char first_char = toupper(book.title[0]);
                return first_char == 'T';
            }
            return false;
        });

    out_file << "5. num of books starting with letter 'T': " << books_starting_with_T << endl;

    out_file.close();
    cout << "results saved to file: " << output_filename << endl;
}

