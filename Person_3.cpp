// Alexis Waldron
#include "Person_4.cpp"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <random>
#include <chrono>
#include <algorithm>
#include <stdexcept>
#include <cctype>
#include "constants.h"
using namespace std;

// ============================================================================
// CONSTANTS - Define all magic numbers at the top
// ============================================================================
// Constants already defined in word_system.cpp
// const int MIN_WORD_LENGTH = 3;
// const int MAX_WORD_LENGTH = 6;
const int DEFAULT_WORD_LENGTH = 5;
const int EASY_MAX_UNIQUE = 4;      // Threshold for EASY (repeated letters)
const size_t MIN_COMMON_WORDS = 50; // Minimum words needed in common file
const size_t MIN_WORD_POOL = 5;     // Minimum words needed for difficulty

// ============================================================================
// ENUMERATIONS AND CONSTANTS - Difficulty level definitions
// ============================================================================

enum Difficulty {
    EASY,
    MEDIUM,
    HARD
};

/**
 * Map difficulty enum to human-readable labels
 */
map<Difficulty, string> difficultyLabel = {
    { EASY,   "easy"   },
    { MEDIUM, "medium" },
    { HARD,   "hard"   }
};

// ============================================================================
// DATA STRUCTURES - Difficulty system state
// ============================================================================

/**
 * State container for difficulty settings
 */
struct DifficultySystem {
    Difficulty currentDifficulty;  // Current difficulty level
    int wordLength;                // Target word length
    int successCount;              // Track successful games
    int failureCount;              // Track failed games
    
    /**
     * Constructor: Initialize with defaults
     */
    DifficultySystem() 
        : currentDifficulty(MEDIUM), wordLength(DEFAULT_WORD_LENGTH),
          successCount(0), failureCount(0) {}
};

// ============================================================================
// DIFFICULTY MANAGEMENT FUNCTIONS - Modular, single-purpose
// ============================================================================

/**
 * Set the current difficulty level
 * Time Complexity: O(1)
 * @param ds Reference to DifficultySystem
 * @param level New difficulty level
 */
void setDifficulty(DifficultySystem& ds, Difficulty level)
{
    ds.currentDifficulty = level;
    cout << "[Difficulty] Set to: "
         << difficultyLabel[ds.currentDifficulty] << "\n";
}

/**
 * Set the target word length
 * Time Complexity: O(1)
 * @param ds Reference to DifficultySystem
 * @param length New word length
 * @throw invalid_argument if length is out of valid range
 */
void setWordLength(DifficultySystem& ds, int length)
{
    if (length < MIN_WORD_LENGTH || length > MAX_WORD_LENGTH) {
        throw invalid_argument(
            "[Difficulty] ERROR: word length must be between " +
            to_string(MIN_WORD_LENGTH) + " and " +
            to_string(MAX_WORD_LENGTH) + ".");
    }
    ds.wordLength = length;
    cout << "[Difficulty] Word length set to: " << ds.wordLength << "\n";
}

/**
 * Record a successful game and potentially increase difficulty
 * Time Complexity: O(1)
 * @param ds Reference to DifficultySystem
 */
void recordSuccess(DifficultySystem& ds)
{
    ds.successCount++;
    cout << "[Difficulty] Success count: " << ds.successCount << "\n";
    
    // Auto-escalate difficulty after 3 consecutive wins
    if (ds.successCount >= 3 && ds.currentDifficulty != HARD) {
        setDifficulty(ds, static_cast<Difficulty>(ds.currentDifficulty + 1));
        ds.successCount = 0;  // Reset counter
    }
}

/**
 * Record a failed game and potentially decrease difficulty
 * Time Complexity: O(1)
 * @param ds Reference to DifficultySystem
 */
void recordFailure(DifficultySystem& ds)
{
    ds.failureCount++;
    cout << "[Difficulty] Failure count: " << ds.failureCount << "\n";
    
    // Auto-deescalate difficulty after 3 consecutive losses
    if (ds.failureCount >= 3 && ds.currentDifficulty != EASY) {
        setDifficulty(ds, static_cast<Difficulty>(ds.currentDifficulty - 1));
        ds.failureCount = 0;  // Reset counter
    }
}

/**
 * Get difficulty label for display
 * Time Complexity: O(1)
 * @param ds Reference to DifficultySystem
 * @return String label for current difficulty
 */
string getDifficultyLabel(const DifficultySystem& ds)
{
    return difficultyLabel[ds.currentDifficulty];
}

/**
 * Print current difficulty settings
 * Time Complexity: O(1)
 * @param ds Reference to DifficultySystem
 */
void printDifficultyStats(const DifficultySystem& ds)
{
    cout << "\n========== DIFFICULTY STATS ==========\n";
    cout << "Current Level: " << getDifficultyLabel(ds) << "\n";
    cout << "Word Length:   " << ds.wordLength << "\n";
    cout << "Successes:     " << ds.successCount << "\n";
    cout << "Failures:      " << ds.failureCount << "\n";
    cout << "=======================================\n\n";
}

// ============================================================================
// FILE I/O FUNCTIONS - Load and validate word lists
// ============================================================================

/**
 * Load common words from file into set
 * Time Complexity: O(n log n) for set insertions
 * Space Complexity: O(n)
 * @param filepath Path to common words file
 * @return Set of common words (lowercase)
 * @throw runtime_error if file cannot be opened
 */
set<string> loadCommonWords(const string& filepath)
{
    set<string> common;
    ifstream file(filepath);
    
    if (!file.is_open()) {
        throw runtime_error(
            "[Difficulty] ERROR: could not open file: " + filepath);
    }

    string word;
    int lineNum = 0;
    while (file >> word) {
        lineNum++;
        try {
            // Convert to lowercase for consistent comparison
            transform(word.begin(), word.end(), word.begin(), ::tolower);
            
            // Validate word contains only alphabetic characters
            bool valid = true;
            for (char c : word) {
                if (!isalpha(c)) {
                    valid = false;
                    break;
                }
            }
            
            if (valid && !word.empty()) {
                common.insert(word);
            }
        } catch (const exception& e) {
            cerr << "[Warning] Error on line " << lineNum << ": " 
                 << e.what() << "\n";
        }
    }

    file.close();

    if (common.empty()) {
        throw runtime_error(
            "[Difficulty] ERROR: common words file is empty or invalid.");
    }

    if (common.size() < MIN_COMMON_WORDS) {
        cerr << "[Warning] Common words file has only " << common.size()
             << " words (expected at least " << MIN_COMMON_WORDS << ")\n";
    }

    cout << "[Difficulty] Loaded " << common.size() 
         << " common words from file.\n";
    return common;
}

// ============================================================================
// WORD DIFFICULTY CLASSIFICATION - Analyze word properties
// ============================================================================

/**
 * Categorize a word by difficulty
 * 
 * Algorithm:
 * 1. If word is in common words list -> EASY
 * 2. Count unique letters
 * 3. If unique letters <= length-1 (has repeats) -> MEDIUM
 * 4. If all letters unique -> HARD
 * 
 * Time Complexity: O(n + 26) = O(n) where n = word length
 * Space Complexity: O(26) = O(1) for boolean array
 * 
 * @param word Word to categorize (lowercase)
 * @param commonWords Set of common words
 * @return Difficulty enum value
 */
Difficulty categorizeWord(const string& word, const set<string>& commonWords)
{
    // Check if word is in common words list
    if (commonWords.count(word)) {
        return EASY;
    }

    // Count unique letters using frequency array
    bool seen[26] = { false };
    int uniqueCount = 0;
    
    for (char c : word) {
        int idx = c - 'a';
        if (!seen[idx]) {
            seen[idx] = true;
            uniqueCount++;
        }
    }

    // Words with repeated letters are MEDIUM
    if (uniqueCount <= (int)word.length() - 1) {
        return MEDIUM;
    }

    // Words with all unique letters are HARD
    return HARD;
}

/**
 * Print word difficulty analysis for debugging
 * Time Complexity: O(n)
 * @param word Word to analyze
 * @param difficulty Calculated difficulty
 */
void printWordDifficulty(const string& word, Difficulty difficulty)
{
    cout << "[Difficulty] \"" << word << "\" -> " 
         << difficultyLabel[difficulty] << "\n";
}

// ============================================================================
// WORD SELECTION FUNCTIONS - Build and select from word pools
// ============================================================================

/**
 * Load words matching current difficulty settings
 * 
 * Algorithm:
 * 1. Load common words for reference
 * 2. Read all words from file
 * 3. Filter by: length matches && difficulty matches
 * 4. Return filtered pool
 * 
 * Time Complexity: O(n * m) where n = words in file, m = avg word length
 * Space Complexity: O(k) where k = words in result pool
 * 
 * @param ds Difficulty system with current settings
 * @param wordFile Path to complete word list
 * @param commonFile Path to common words list
 * @return Vector of words matching difficulty criteria
 */
vector<string> loadWordPool(const DifficultySystem& ds,
                            const string& wordFile,
                            const string& commonFile)
{
    vector<string> pool;
    
    // Load reference set of common words
    set<string> commonWords;
    try {
        commonWords = loadCommonWords(commonFile);
    } catch (const exception& e) {
        cerr << "[Difficulty] ERROR loading common words: " << e.what() << "\n";
        return pool;
    }

    // Open word file
    ifstream file(wordFile);
    if (!file.is_open()) {
        cerr << "[Difficulty] ERROR: could not open " << wordFile << "\n";
        return pool;
    }

    string word;
    int lineNum = 0;
    while (file >> word) {
        lineNum++;
        try {
            // Convert to lowercase
            transform(word.begin(), word.end(), word.begin(), ::tolower);

            // Filter by length
            if ((int)word.length() != ds.wordLength) {
                continue;
            }

            // Categorize and filter by difficulty
            Difficulty wordDifficulty = categorizeWord(word, commonWords);
            if (wordDifficulty == ds.currentDifficulty) {
                pool.push_back(word);
            }
        } catch (const exception& e) {
            cerr << "[Warning] Error on line " << lineNum << ": " 
                 << e.what() << "\n";
        }
    }

    file.close();

    if (pool.empty()) {
        cerr << "[Difficulty] WARNING: No words found for "
             << "length=" << ds.wordLength 
             << " difficulty=" << difficultyLabel[ds.currentDifficulty] << "\n";
    }

    return pool;
}

/**
 * Select a random word matching difficulty settings
 * 
 * Algorithm:
 * 1. Load word pool for current settings
 * 2. Validate pool is not empty
 * 3. Generate random index
 * 4. Return word at that index
 * 
 * Time Complexity: O(n) for loading pool
 * Space Complexity: O(n) for pool vector
 * 
 * @param ds Difficulty system
 * @param wordFile Path to word list
 * @param commonFile Path to common words
 * @return Selected word, or empty string if no words available
 */
string selectWordForDifficulty(const DifficultySystem& ds,
                               const string& wordFile,
                               const string& commonFile)
{
    vector<string> pool = loadWordPool(ds, wordFile, commonFile);

    if (pool.empty()) {
        cerr << "[Difficulty] ERROR: word pool is empty.\n";
        return "";
    }

    if (pool.size() < MIN_WORD_POOL) {
        cerr << "[Warning] Word pool has only " << pool.size() 
             << " words (minimum recommended: " << MIN_WORD_POOL << ")\n";
    }

    // Generate random index using high-resolution clock seed
    mt19937 rng(static_cast<unsigned>(
        chrono::high_resolution_clock::now().time_since_epoch().count()));
    uniform_int_distribution<size_t> dist(0, pool.size() - 1);
    
    return pool[dist(rng)];
}

/**
 * Select multiple words for batch processing
 * Time Complexity: O(n * m) where n = words needed, m = avg loading time
 * @param ds Difficulty system
 * @param wordFile Path to word list
 * @param commonFile Path to common words
 * @param count Number of words to select
 * @return Vector of selected words
 */
vector<string> selectWordsForDifficulty(const DifficultySystem& ds,
                                       const string& wordFile,
                                       const string& commonFile,
                                       size_t count)
{
    vector<string> result;
    vector<string> pool = loadWordPool(ds, wordFile, commonFile);

    if (pool.empty()) {
        cerr << "[Difficulty] ERROR: word pool is empty.\n";
        return result;
    }

    // Ensure we don't request more words than available
    size_t selectCount = min(count, pool.size());

    mt19937 rng(static_cast<unsigned>(
        chrono::high_resolution_clock::now().time_since_epoch().count()));

    // Select unique words
    set<size_t> selectedIndices;
    while (selectedIndices.size() < selectCount) {
        uniform_int_distribution<size_t> dist(0, pool.size() - 1);
        selectedIndices.insert(dist(rng));
    }

    for (size_t idx : selectedIndices) {
        result.push_back(pool[idx]);
    }

    return result;
}
