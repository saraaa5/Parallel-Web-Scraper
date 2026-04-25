#include "Indexer.h"
#include "Downloader.h"
#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>
#include <iostream>
#include <algorithm>
#include <tbb/concurrent_set.h>

using namespace std;
using namespace tbb;


concurrent_vector<string> discover_book_pages(int max_pages) {
    concurrent_vector<string> all_book_urls;
    concurrent_set<string> visited_categories;

    cout << "[INDEXER] Starting automatic indexing of " << base_url_ << endl;

    try {
        // Preuzmi početnu stranicu
        string index_html = download_page(base_url_ + "/index.html");
        cout << "[INDEXER] Downloaded index page (" << index_html.size() << " bytes)" << endl;

        // Koristi postojeći Gumbo parser za ekstrakciju kategorija
        GumboOutput* output = gumbo_parse(index_html.c_str());

        // Pronađi sve linkove ka kategorijama
        auto category_links = extract_links_from_node(output->root);

        // Filtriraj samo linkove ka kategorijama knjiga
        for (const auto& link : category_links) {
            if (link.find("category/books/") != string::npos ||
                link.find("catalogue/category/") != string::npos) {
                string full_url = make_absolute_url(link);
                if (visited_categories.find(full_url) == visited_categories.end()) {
                    visited_categories.insert(full_url);
                }
            }
        }

        gumbo_destroy_output(&kGumboDefaultOptions, output);

        cout << "[INDEXER] Found " << visited_categories.size() << " categories to scan" << endl;

        // Paralelno obradi sve kategorije
        vector<string> categories(visited_categories.begin(), visited_categories.end());

        parallel_for(blocked_range<size_t>(0, categories.size()),
            [&](const blocked_range<size_t>& range) {
                for (size_t i = range.begin(); i < range.end(); ++i) {
                    if (all_book_urls.size() >= max_pages) {
                        return; // Prekini ako smo dostigli limit
                    }

                    try {
                        auto book_urls = get_category_book_urls(categories[i]);
                        cout << "[INDEXER] Category " << i + 1 << "/" << categories.size()
                            << ": " << book_urls.size() << " books" << endl;

                        for (const auto& url : book_urls) {
                            if (all_book_urls.size() < max_pages) {
                                all_book_urls.push_back(url);
                            }
                        }
                    }
                    catch (const exception& e) {
                        cerr << "[INDEXER ERROR] Category " << categories[i] << ": " << e.what() << endl;
                    }
                }
            });

    }
    catch (const exception& e) {
        cerr << "[INDEXER ERROR] " << e.what() << endl;
    }

    cout << "[INDEXER] Indexing complete. Found " << all_book_urls.size() << " book pages." << endl;
    return all_book_urls;
}

concurrent_vector<string> get_category_book_urls(const string& category_url) {
    concurrent_vector<string> book_urls;

    try {
        string html = download_page(category_url);
        GumboOutput* output = gumbo_parse(html.c_str());

        // Pronađi sve article elemente sa knjigama
        auto article_nodes = find_nodes_by_tag_and_class(output->root, GUMBO_TAG_ARTICLE, "product_pod");

        for (GumboNode* article : article_nodes) {
            // Unutar svakog article, pronadji link ka detaljnoj stranici knjige
            auto links = extract_links_from_node(article);
            for (const auto& link : links) {
                if (link.find("catalogue/") != string::npos &&
                    link.find("index.html") != string::npos) {
                    book_urls.push_back(make_absolute_url(link));
                    break; // Jedan link po knjizi je dovoljan
                }
            }
        }

        // Proveri da li postoje dodatne stranice (paginacija)
        auto pagination_links = find_pagination_links(output->root);
        if (!pagination_links.empty()) {
            cout << "[INDEXER] Found " << pagination_links.size() << " additional pages in category" << endl;

            // Obradi dodatne stranice (možeš dodati paralelizam i ovde ako želiš)
            for (const auto& page_link : pagination_links) {
                try {
                    string page_url = make_absolute_url(page_link);
                    string page_html = download_page(page_url);
                    GumboOutput* page_output = gumbo_parse(page_html.c_str());

                    auto page_articles = find_nodes_by_tag_and_class(page_output->root, GUMBO_TAG_ARTICLE, "product_pod");
                    for (GumboNode* article : page_articles) {
                        auto links = extract_links_from_node(article);
                        for (const auto& link : links) {
                            if (link.find("catalogue/") != string::npos) {
                                book_urls.push_back(make_absolute_url(link));
                                break;
                            }
                        }
                    }

                    gumbo_destroy_output(&kGumboDefaultOptions, page_output);
                }
                catch (const exception& e) {
                    cerr << "[INDEXER ERROR] Page " << page_link << ": " << e.what() << endl;
                }
            }
        }

        gumbo_destroy_output(&kGumboDefaultOptions, output);

    }
    catch (const exception& e) {
        cerr << "[INDEXER ERROR] Category " << category_url << ": " << e.what() << endl;
    }

    return book_urls;
}

string make_absolute_url(const string& relative_url) {
    if (relative_url.find("http") == 0) {
        return relative_url;
    }

    if (relative_url[0] == '/') {
        return base_url_ + relative_url;
    }

    // Ukloni "../" ako postoje u relativnoj putanji
    string clean_url = relative_url;
    size_t pos;
    while ((pos = clean_url.find("../")) == 0) {
        clean_url = clean_url.substr(3);
    }

    return base_url_ + "/" + clean_url;
}