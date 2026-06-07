// Jayden Fergus
#include "constants.h"
#include <vector>
#include <string>
#include <iostream>
#include <map>
#include <array>
#include <cctype>
#include <stdexcept>
using namespace std;

// ============================================================================
// CONSTANTS - Define all magic numbers
// ============================================================================
// Constants already defined in word_system.cpp
// const int MAX_WORD_LENGTH = 6;
// const int MIN_WORD_LENGTH = 3;

// ============================================================================
// DATA STRUCTURES - Letter result representation
// ============================================================================

/**
 * Result of validating a single letter in a guess
 */
struct LetterResult {
    char letter;      // The letter being validated
    string color;     // Color code: "green", "yellow", "grey", "pending"
};

// ============================================================================
// UTILITY FUNCTIONS - Helper functions for validation
// ============================================================================

/**
 * Build frequency map of all letters in a word
 * Time Complexity: O(n) where n = word length
 * Space Complexity: O(26) = O(1) for alphabetic characters
 * @param word Input word
 * @return Map of character frequencies
 */
map<char, int> buildLetterFrequency(const string& word)
{
    map<char, int> freq;
    for (char c : word) {
        freq[c]++;
    }
    return freq;
}

/**
 * Convert string to lowercase
 * Time Complexity: O(n)
 * Space Complexity: O(n)
 * @param word Input word
 * @return Lowercase word
 */
string toLower(string word)
{
    for (char& c : word) {
        c = tolower(c);
    }
    return word;
}

/**
 * Validate that word contains only alphabetic characters
 * Time Complexity: O(n)
 * @param word Word to validate
 * @return True if all characters are alphabetic
 */
bool isValidWord(const string& word)
{
    if (word.empty()) return false;
    
    for (char c : word) {
        if (!isalpha(c)) {
            return false;
        }
    }
    return true;
}

/**
 * Check if word length is within acceptable range
 * Time Complexity: O(1)
 * @param len Length to check
 * @return True if length is valid
 */
// isValidLength already defined in word_system.cpp
/*
bool isValidLength(size_t len)
{
    return len >= MIN_WORD_LENGTH && len <= MAX_WORD_LENGTH;
}
*/

// ============================================================================
// MAIN VALIDATION ALGORITHM - Wordle-style guess validation
// ============================================================================

/**
 * Validate a guess against the target word
 * 
 * Algorithm:
 * 1. First pass: Mark all exact matches (green)
 * 2. Build frequency map from target, subtract exact matches
 * 3. Second pass: Mark remaining letters as yellow or grey
 * 
 * Time Complexity: O(n + m) where n = word length, m = alphabet size (26)
 * Space Complexity: O(m) for frequency map
 * 
 * Color meanings:
 * - "green"   : Correct letter in correct position
 * - "yellow"  : Correct letter in wrong position
 * - "grey"    : Letter not in target word
 * - "pending" : Intermediate state during processing
 * 
 * Front-end usage:
 * Call validateGuess(guess, targetWord) after each guess submission.
 * Loop through returned vector and use .color to set tile background colors:
 *   "green"  = correct letter, correct position
 *   "yellow" = correct letter, wrong position
 *   "grey"   = letter not in the word
 * Use .letter to identify which tile to color
 * 
 * @param guess The player's guess (case-insensitive)
 * @param targetWord The target word to match (case-insensitive)
 * @return Vector of LetterResult with color for each guessed letter
 * @throw invalid_argument if word contains non-alphabetic characters
 * @throw length_error if guess length doesn't match target
 */
vector<LetterResult> validateGuess(string guess, string targetWord)
{
    vector<LetterResult> results;

    // INPUT VALIDATION
    // ================================================================
    
    // Convert to lowercase for comparison
    guess = toLower(guess);
    targetWord = toLower(targetWord);

    // Check that both words contain only alphabetic characters
    if (!isValidWord(guess)) {
        throw invalid_argument(
            "[Validation] Guess contains non-alphabetic characters.");
    }

    if (!isValidWord(targetWord)) {
        throw invalid_argument(
            "[Validation] Target word contains non-alphabetic characters.");
    }

    // Check word lengths match
    if (guess.length() != targetWord.length()) {
        throw length_error(
            "[Validation] Invalid guess length. Expected " + 
            to_string(targetWord.length()) + " letters, got " +
            to_string(guess.length()) + ".");
    }

    // Check word length is within acceptable range
    if (!isValidLength(guess.length())) {
        throw length_error(
            "[Validation] Word length " + to_string(guess.length()) +
            " is outside valid range [" + to_string(MIN_WORD_LENGTH) + 
            ", " + to_string(MAX_WORD_LENGTH) + "].");
    }

    int length = targetWord.length();

    // FIRST PASS: Identify exact matches (green)
    // ================================================================
    // Time: O(n), Space: O(n) for results vector
    
    array<bool, MAX_WORD_LENGTH> matched = {false};

    for (int i = 0; i < length; i++) {
        LetterResult res;
        res.letter = guess[i];

        if (guess[i] == targetWord[i]) {
            res.color = "green";
            matched[i] = true;
        } else {
            res.color = "pending";
        }

        results.push_back(res);
    }

    // BUILD FREQUENCY MAP: Count available letters in target
    // ================================================================
    // Time: O(n), Space: O(26) = O(1)
    
    map<char, int> freq = buildLetterFrequency(targetWord);

    // Subtract letters that were matched exactly
    // Time: O(n)
    for (int i = 0; i < length; i++) {
        if (results[i].color == "green") {
            freq[targetWord[i]]--;
        }
    }

    // SECOND PASS: Identify yellows and greys
    // ================================================================
    // Time: O(n)
    
    for (int i = 0; i < length; i++) {
        // Skip already-colored (green) letters
        if (results[i].color == "green") {
            continue;
        }

        char c = guess[i];

        // Check if letter exists in remaining frequency pool
        if (freq.count(c) && freq[c] > 0) {
            results[i].color = "yellow";
            freq[c]--;
        } else {
            results[i].color = "grey";
        }
    }

    return results;
}

/**
 * Print validation results for debugging
 * Time Complexity: O(n)
 * @param results Vector of letter results
 * @param word The word being analyzed
 */
void printValidationResults(const vector<LetterResult>& results,
                           const string& word)
{
    cout << "[Validation] Results for \"" << word << "\":\n";
    for (const auto& res : results) {
        cout << "  " << res.letter << " -> " << res.color << "\n";
    }
}

/**
 * Count letters by color for statistics
 * Time Complexity: O(n)
 * @param results Vector of letter results
 * @return Map of color -> count
 */
map<string, int> countByColor(const vector<LetterResult>& results)
{
    map<string, int> counts;
    for (const auto& res : results) {
        counts[res.color]++;
    }
    return counts;
}
