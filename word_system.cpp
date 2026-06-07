// Naeem Augustine
#include "constants.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <random>
#include <chrono>
#include <numeric>
#include <stdexcept>
#include <cctype>
#include <map>
#include <set>
using namespace std;

// ============================================================================
// CONSTANTS - All magic numbers defined at top
// ============================================================================
const size_t MIN_WORD_LENGTH = 3;
const size_t MAX_WORD_LENGTH = 6;
const size_t MIN_FILE_SIZE = 100;  // Minimum expected words in file
const int MAX_LOAD_ATTEMPTS = 3;
const char COMMENT_CHAR = '#';

// ============================================================================
// UTILITY FUNCTIONS - Modular, reusable helper functions
// ============================================================================

/**
 * Trim whitespace from both ends of a string
 * @param s Input string
 * @return Trimmed string
 */
static string trim(const string& s)
{
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == string::npos) return {};
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

/**
 * Convert string to lowercase
 * @param s Input string
 * @return Lowercase string
 */
static string toLowerWord(string s)
{
    transform(s.begin(), s.end(), s.begin(),
              [](unsigned char c) { return tolower(c); });
    return s;
}

/**
 * Validate that string contains only alphabetic characters
 * @param s Input string
 * @return True if all characters are alphabetic
 */
static bool isAlphaOnly(const string& s)
{
    if (s.empty()) return false;
    for (unsigned char c : s)
        if (!isalpha(c)) return false;
    return true;
}

/**
 * Validate word length is within acceptable range
 * @param len Length to validate
 * @return True if length is valid
 */
static bool isValidLength(size_t len)
{
    return len >= MIN_WORD_LENGTH && len <= MAX_WORD_LENGTH;
}

// ============================================================================
// SEARCH ALGORITHMS - Binary Search Implementation
// ============================================================================

/**
 * Binary Search: Find word in sorted list
 * Time Complexity: O(log n)
 * Space Complexity: O(1)
 * @param words Sorted vector of words
 * @param target Word to find
 * @return Index if found, -1 if not found
 */
static int binarySearch(const vector<string>& words, const string& target)
{
    int left = 0, right = words.size() - 1;
    
    while (left <= right) {
        int mid = left + (right - left) / 2;
        int cmp = words[mid].compare(target);
        
        if (cmp == 0) {
            return mid;  // Found
        } else if (cmp < 0) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    return -1;  // Not found
}

/**
 * Linear Search: Find word in unsorted list
 * Time Complexity: O(n)
 * Space Complexity: O(1)
 * @param words Vector of words
 * @param target Word to find
 * @return Index if found, -1 if not found
 */
static int linearSearch(const vector<string>& words, const string& target)
{
    for (size_t i = 0; i < words.size(); ++i) {
        if (words[i] == target) {
            return i;
        }
    }
    return -1;
}

// ============================================================================
// SORTING ALGORITHM - Merge Sort Implementation
// ============================================================================

/**
 * Merge two sorted subarrays
 * Time Complexity: O(n)
 * Space Complexity: O(n)
 */
static void merge(vector<string>& arr, int left, int mid, int right)
{
    vector<string> temp;
    int i = left, j = mid + 1;
    
    while (i <= mid && j <= right) {
        if (arr[i] <= arr[j]) {
            temp.push_back(arr[i++]);
        } else {
            temp.push_back(arr[j++]);
        }
    }
    
    while (i <= mid) {
        temp.push_back(arr[i++]);
    }
    
    while (j <= right) {
        temp.push_back(arr[j++]);
    }
    
    for (size_t i = 0; i < temp.size(); ++i) {
        arr[left + i] = temp[i];
    }
}

/**
 * Merge Sort: Sort array in ascending order
 * Time Complexity: O(n log n)
 * Space Complexity: O(n)
 * @param arr Array to sort
 * @param left Start index
 * @param right End index
 */
static void mergeSort(vector<string>& arr, int left, int right)
{
    if (left < right) {
        int mid = left + (right - left) / 2;
        mergeSort(arr, left, mid);
        mergeSort(arr, mid + 1, right);
        merge(arr, left, mid, right);
    }
}

// ============================================================================
// WORD SYSTEM CLASS - Main data structure and operations
// ============================================================================

class WordSystem
{
public:
    /**
     * Constructor: Initialize word system with file and parameters
     * @param filepath Path to word dictionary file
     * @param minLen Minimum word length (default 3)
     * @param maxLen Maximum word length (default 6)
     */
    WordSystem(const string& filepath,
               size_t minLen = MIN_WORD_LENGTH,
               size_t maxLen = MAX_WORD_LENGTH)
        : m_filepath(filepath), m_minLen(minLen), m_maxLen(maxLen),
          m_isFiltered(false), m_isSorted(false), m_totalAttempts(0)
    {
        validateLengthRange();
        loadFromFile();
        buildFilteredList();
        initializeRNG();
    }

    // ========================================================================
    // PUBLIC ACCESSORS - Query methods with error checking
    // ========================================================================

    size_t totalLoaded()   const { return m_allWords.size(); }
    size_t totalFiltered() const { return m_filtered.size(); }
    size_t minLength()     const { return m_minLen;          }
    size_t maxLength()     const { return m_maxLen;          }
    bool isFiltered()      const { return m_isFiltered;      }
    bool isSorted()        const { return m_isSorted;        }
    int getTotalAttempts() const { return m_totalAttempts;   }

    const vector<string>& filteredWords() const { return m_filtered; }
    const vector<string>& sortedWords()   const { return m_sorted;   }

    // ========================================================================
    // WORD SELECTION METHODS
    // ========================================================================

    /**
     * Get a random word from filtered list
     * Time Complexity: O(1)
     * @return Random word
     * @throw runtime_error if filtered list is empty
     */
    string randomWord()
    {
        if (m_filtered.empty())
            throw runtime_error("Filtered word list is empty.");
        uniform_int_distribution<size_t> dist(0, m_filtered.size() - 1);
        return m_filtered[dist(m_rng)];
    }

    /**
     * Get a random word of specific length
     * Time Complexity: O(1)
     * @param len Desired word length
     * @return Random word of specified length
     * @throw runtime_error if no words of that length exist
     */
    string randomWordOfLength(size_t len)
    {
        if (!isValidLength(len)) {
            throw out_of_range("Length " + to_string(len) + 
                             " out of range [" + to_string(m_minLen) + 
                             ", " + to_string(m_maxLen) + "]");
        }

        auto it = m_byLength.find(len);
        if (it == m_byLength.end() || it->second.empty())
            throw runtime_error(
                "No words of length " + to_string(len) + " in filtered list.");
        
        uniform_int_distribution<size_t> dist(0, it->second.size() - 1);
        return it->second[dist(m_rng)];
    }

    /**
     * Get multiple random words without repetition
     * Time Complexity: O(n log n) due to shuffling
     * Space Complexity: O(n)
     * @param n Number of words to retrieve
     * @return Vector of unique random words
     * @throw out_of_range if n exceeds available words
     */
    vector<string> randomWords(size_t n)
    {
        if (n > m_filtered.size())
            throw out_of_range(
                "Requested " + to_string(n) +
                " words but only " + to_string(m_filtered.size()) +
                " are available.");

        vector<size_t> indices(m_filtered.size());
        iota(indices.begin(), indices.end(), 0);
        shuffle(indices.begin(), indices.end(), m_rng);

        vector<string> result;
        result.reserve(n);
        for (size_t i = 0; i < n; ++i)
            result.push_back(m_filtered[indices[i]]);
        return result;
    }

    // ========================================================================
    // SEARCH OPERATIONS - Find words efficiently
    // ========================================================================

    /**
     * Binary search for word in sorted list (requires sortWords() first)
     * Time Complexity: O(log n)
     * @param word Word to search for
     * @return True if word exists
     */
    bool binarySearchWord(const string& word)
    {
        if (!m_isSorted) {
            cerr << "[Warning] Call sortWords() before binary search.\n";
            return false;
        }
        return binarySearch(m_sorted, toLowerWord(word)) != -1;
    }

    /**
     * Linear search for word (works on unsorted list)
     * Time Complexity: O(n)
     * @param word Word to search for
     * @return True if word exists
     */
    bool linearSearchWord(const string& word)
    {
        return linearSearch(m_filtered, toLowerWord(word)) != -1;
    }

    // ========================================================================
    // SORTING OPERATION - Sort words using merge sort
    // ========================================================================

    /**
     * Sort filtered words using merge sort algorithm
     * Time Complexity: O(n log n)
     * Space Complexity: O(n)
     */
    void sortWords()
    {
        if (m_isSorted) {
            cout << "[WordSystem] Words already sorted.\n";
            return;
        }

        m_sorted = m_filtered;
        if (!m_sorted.empty()) {
            mergeSort(m_sorted, 0, m_sorted.size() - 1);
            m_isSorted = true;
            cout << "[WordSystem] Sorted " << m_sorted.size() 
                 << " words using merge sort.\n";
        }
    }

    // ========================================================================
    // STATISTICS AND REPORTING
    // ========================================================================

    /**
     * Print comprehensive statistics about loaded words
     */
    void printStats() const
    {
        cout << "\n========== WORD SYSTEM STATISTICS ==========\n";
        cout << "Total loaded:   " << totalLoaded()   << "\n";
        cout << "Total filtered: " << totalFiltered() << "\n";
        cout << "Is filtered:    " << (m_isFiltered ? "Yes" : "No") << "\n";
        cout << "Is sorted:      " << (m_isSorted ? "Yes" : "No") << "\n";
        cout << "Length range:   " << m_minLen << " - " << m_maxLen << "\n";
        cout << "Total attempts: " << m_totalAttempts << "\n";
        cout << "\nWords by length:\n";
        for (size_t len = m_minLen; len <= m_maxLen; ++len)
        {
            auto it = m_byLength.find(len);
            size_t cnt = (it != m_byLength.end()) ? it->second.size() : 0;
            cout << "  " << len << " letters: " << cnt << "\n";
        }
        cout << "==========================================\n\n";
    }

    /**
     * Increment attempt counter for tracking
     */
    void incrementAttempts()
    {
        ++m_totalAttempts;
    }

private:
    // ========================================================================
    // PRIVATE INITIALIZATION METHODS
    // ========================================================================

    /**
     * Validate that min and max lengths are sensible
     * @throw invalid_argument if range is invalid
     */
    void validateLengthRange()
    {
        if (m_minLen > m_maxLen) {
            throw invalid_argument("minLen cannot be greater than maxLen");
        }
        if (m_maxLen > MAX_WORD_LENGTH) {
            throw invalid_argument("maxLen exceeds maximum allowed length");
        }
    }

    /**
     * Initialize random number generator with current time
     */
    void initializeRNG()
    {
        auto seed = static_cast<unsigned>(
            chrono::high_resolution_clock::now()
                .time_since_epoch().count());
        m_rng.seed(seed);
    }

    /**
     * Load all words from file with error handling
     * Time Complexity: O(n) where n = number of lines
     * @throw runtime_error if file cannot be opened
     * @throw runtime_error if file is too small
     */
    void loadFromFile()
    {
        ifstream file(m_filepath);
        if (!file.is_open()) {
            throw runtime_error("Cannot open file: " + m_filepath);
        }

        string line;
        int lineNum = 0;
        while (getline(file, line)) {
            lineNum++;
            try {
                string word = trim(line);
                if (!word.empty() && word[0] != COMMENT_CHAR) {
                    m_allWords.push_back(word);
                }
            } catch (const exception& e) {
                cerr << "[Warning] Error on line " << lineNum << ": " 
                     << e.what() << "\n";
            }
        }

        file.close();

        if (m_allWords.empty()) {
            throw runtime_error("Word file is empty or contains no valid words.");
        }

        if (m_allWords.size() < MIN_FILE_SIZE) {
            cerr << "[Warning] Word file has " << m_allWords.size() 
                 << " words (expected at least " << MIN_FILE_SIZE << ")\n";
        }

        cout << "[WordSystem] Loaded " << m_allWords.size() 
             << " total words from file.\n";
    }

    /**
     * Build filtered list and index by length
     * Time Complexity: O(n) where n = total words
     * Space Complexity: O(n)
     */
    void buildFilteredList()
    {
        m_filtered.clear();
        m_byLength.clear();

        for (const auto& raw : m_allWords) {
            if (!isAlphaOnly(raw)) continue;
            
            string w = toLowerWord(raw);
            if (!isValidLength(w.size())) continue;

            m_filtered.push_back(w);
            m_byLength[w.size()].push_back(w);
        }

        m_isFiltered = true;

        if (m_filtered.empty()) {
            cerr << "[Error] No words matched the filter criteria.\n";
            throw runtime_error("Filtered list is empty after processing.");
        }

        cout << "[WordSystem] Filtered to " << m_filtered.size() 
             << " valid words.\n";
    }

    // ========================================================================
    // PRIVATE MEMBER VARIABLES - Data structures
    // ========================================================================

    string m_filepath;              // Path to dictionary file
    size_t m_minLen;                // Minimum word length
    size_t m_maxLen;                // Maximum word length
    
    vector<string> m_allWords;      // All words loaded from file
    vector<string> m_filtered;      // Filtered words (valid length)
    vector<string> m_sorted;        // Sorted copy of filtered words
    map<size_t, vector<string>> m_byLength;  // Words indexed by length
    
    mt19937 m_rng;                  // Random number generator
    
    bool m_isFiltered;              // Has buildFilteredList been called?
    bool m_isSorted;                // Has sortWords been called?
    int m_totalAttempts;            // Track total selection attempts
};
