#include "GumboParser.h"
#include <gumbo.h>
#include <string>
#include <vector>
#include <cctype>
#include <iostream>
using namespace std;

//rekurzivno prolazi kroz cvorove HTML stabla i prikuplja sav tekstualni sadrzj
string get_text(GumboNode* node) {
    if (node->type == GUMBO_NODE_TEXT) {
        return string(node->v.text.text);
    }
    if (node->type == GUMBO_NODE_ELEMENT) {
        string result;
        GumboVector* children = &node->v.element.children;
        for (unsigned int i = 0; i < children->length; ++i) {
            result += get_text(static_cast<GumboNode*>(children->data[i]));
        }
        return result;
    }
    return "";
}

int stars_from_class(const string& s) {
    if (s.find("One") != string::npos) return 1;
    if (s.find("Two") != string::npos) return 2;
    if (s.find("Three") != string::npos) return 3;
    if (s.find("Four") != string::npos) return 4;
    if (s.find("Five") != string::npos) return 5;
    return 0;
}

int parse_stars(GumboNode* child) {
    GumboAttribute* classAttr2 = gumbo_get_attribute(&child->v.element.attributes, "class");
    if (classAttr2 && string(classAttr2->value).find("star-rating") != string::npos) {
        return stars_from_class(classAttr2->value);
    }
    return 0;
}

string parse_title(GumboNode* child) {
    GumboNode* a = static_cast<GumboNode*>(child->v.element.children.data[0]);
    if (a->type == GUMBO_NODE_ELEMENT && a->v.element.tag == GUMBO_TAG_A) {
        GumboAttribute* title = gumbo_get_attribute(&a->v.element.attributes, "title");
        if (title) return title->value;
    }
    return "";
}

double parse_price(GumboNode* child) {
    GumboAttribute* divClass = gumbo_get_attribute(&child->v.element.attributes, "class");
    if (divClass && string(divClass->value).find("product_price") != string::npos) {
        GumboVector* priceChildren = &child->v.element.children;
        for (unsigned int j = 0; j < priceChildren->length; ++j) {
            GumboNode* priceNode = static_cast<GumboNode*>(priceChildren->data[j]);
            if (priceNode->type == GUMBO_NODE_ELEMENT &&
                priceNode->v.element.tag == GUMBO_TAG_P) {
                GumboAttribute* pClass = gumbo_get_attribute(&priceNode->v.element.attributes, "class");
                if (pClass && string(pClass->value).find("price_color") != string::npos) {
                    string price_text = get_text(priceNode);

                    string clean_price;
                    for (char c : price_text) {
                        if ((c >= '0' && c <= '9') || c == '.') {
                            clean_price += c;
                        }
                    }
                    if (!clean_price.empty()) {
                        try {
                            return stod(clean_price);
                        }
                        catch (const exception& e) {
                            cerr << "error while parsing price: " << clean_price << " - " << e.what() << endl;
                            return 0.0;
                        }
                    }
                }
            }
        }
    }
    return 0.0;
}

bool parse_availability(GumboNode* child) {
    GumboAttribute* divClass = gumbo_get_attribute(&child->v.element.attributes, "class");
    if (divClass && string(divClass->value).find("product_price") != string::npos) {
        GumboVector* priceChildren = &child->v.element.children;
        for (unsigned int j = 0; j < priceChildren->length; ++j) {
            GumboNode* availabilityNode = static_cast<GumboNode*>(priceChildren->data[j]);
            if (availabilityNode->type == GUMBO_NODE_ELEMENT &&
                availabilityNode->v.element.tag == GUMBO_TAG_P) {
                GumboAttribute* pClass = gumbo_get_attribute(&availabilityNode->v.element.attributes, "class");
                if (pClass && string(pClass->value).find("instock") != string::npos) {
                    string availability_text = get_text(availabilityNode);
                    return availability_text.find("In stock") != string::npos;
                }
            }
        }
    }
    return false;
}

Book parse_book_from_article(GumboNode* article) {
    Book b;
    b.in_stock = false;

    GumboVector* children = &article->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i) {
        GumboNode* child = static_cast<GumboNode*>(children->data[i]);
        if (child->type != GUMBO_NODE_ELEMENT) continue;

        if (child->v.element.tag == GUMBO_TAG_H3) {
            b.title = parse_title(child);
        }
        else if (child->v.element.tag == GUMBO_TAG_DIV) {
            b.price = parse_price(child);
            b.in_stock = parse_availability(child);
        }
        else if (child->v.element.tag == GUMBO_TAG_P) {
            b.stars = parse_stars(child);
        }
    }

    return b;
}

//	Pronalazi <article> elemente sa klasom "product_pod"
// Za svaki <article> poziva parse_book_from_article
vector<Book> parse_books_gumbo(const string& html) {
    vector<Book> books;

    GumboOutput* output = gumbo_parse(html.c_str());
    vector<GumboNode*> stack;
    stack.push_back(output->root);

    while (!stack.empty()) {
        GumboNode* node = stack.back();
        stack.pop_back();

        if (node->type == GUMBO_NODE_ELEMENT &&
            node->v.element.tag == GUMBO_TAG_ARTICLE) {

            GumboAttribute* classAttr = gumbo_get_attribute(&node->v.element.attributes, "class");
            if (classAttr && string(classAttr->value).find("product_pod") != string::npos) {
                Book book = parse_book_from_article(node);
                if (!book.title.empty()) {
                    books.push_back(book);
                }
            }
        }
        if (node->type == GUMBO_NODE_ELEMENT) {
            GumboVector* children = &node->v.element.children;
            for (int i = children->length - 1; i >= 0; --i) {
                stack.push_back(static_cast<GumboNode*>(children->data[i]));
            }
        }
    }

    gumbo_destroy_output(&kGumboDefaultOptions, output);
    return books;
}
