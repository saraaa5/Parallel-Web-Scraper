#pragma once
#include <string>
#include <vector>
#include "Models.h"
#include <gumbo.h>

std::vector<Book> parse_books_gumbo(const std::string& html);