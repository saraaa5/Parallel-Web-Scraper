# Parallel Web Scraper (Intel TBB)

**Parallel Web Scraper** is a high-performance C++ application designed to efficiently extract and analyze data from web pages. The project leverages the **Intel Threading Building Blocks (TBB)** framework to implement a task-based parallelism model, optimizing execution for modern multi-core processors.

## 🎯 Project Objective
The primary goal is to crawl and analyze the [Books to Scrape](https://books.toscrape.com/index.html) website. The system focuses on parallel HTML analysis, thread-safe data storage, and scalable execution.

## 🚀 Key Features
- **Concurrent Downloading:** Simultaneous retrieval of web pages via HTTP/HTTPS requests with built-in timeout and retry logic.
- **Parallel Analysis:** Distributed extraction of metadata and content using TBB task groups and parallel algorithms.
- **Thread-Safe Storage:** Use of TBB concurrent containers to safely manage visited URLs and processed metadata across multiple threads.
- **Scalability:** Performance scales dynamically with the number of CPU cores, with configurable parallelism levels.

## 📊 Analytics & Results
The program performs deep content analysis and outputs the following metrics to a result file:
1. **Total Book Count:** Number of books with a 5-star rating.
2. **Pricing Data:** Average price of books across analyzed pages.
3. **Execution Statistics:** Throughput (pages analyzed per unit of time), total pages downloaded, and unique URL counts.
4. **Custom Metrics:** Three additional user-defined data points extracted during analysis.

## 🛠 Advanced Implementations
This project explores advanced TBB concepts, including:
- **TBB Task Groups:** For efficient task scheduling.
- **Parallel Pipeline:** (Bonus) Use of `tbb::parallel_pipeline` to chain stages: URL Retrieval → Content Analysis → Result Storage.
- **Automatic Indexing:** (Bonus) Parallel site indexing to autonomously discover new pages for analysis.

## 💻 Setup & Usage
- **Requirements:** C++ Compiler, Intel TBB library, and an HTTP library (e.g., libcurl).
- **Configuration:** The application accepts a list of starting URLs as input.
- **Running:**
  ```bash
  ParallelWebScraper.exe <url_1> <url_2> ...
