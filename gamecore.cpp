// Keyon Bertrand
// extra comments have been added by the request of miss and I also added more data structures sicne miss requested it
#include "Person_4.cpp"
#include "Person_3.cpp"
#include <iostream>
#include <string>
#include <vector>
#include <stack>
#include <queue>
#include <deque>
#include <chrono>
#include <stdexcept>
#include "constants.h"
using namespace std;

// ============================================================================
// CONSTANTS - Defines all game parameters
// ============================================================================
const int MAX_ATTEMPTS = 6;
const int LOSS_STREAK_LIMIT = 3;
const int MIN_GUESS_LENGTH = 3;
const int MAX_GUESS_LENGTH = 6;
const int MAX_GAME_HISTORY = 100;

// ============================================================================
// DATA STRUCTURES - this is the game state and records
// ============================================================================

/**
 * just a record of a single guess with validation results
 */
struct GuessRecord {
    string word;                           // This is the guessed word
    bool correct;                          // check for if it's correct
    vector<LetterResult> letterResults;    // color coding for letters
    chrono::system_clock::time_point timestamp;  // tracks shen it was guessed
};

/**
 * Statistics tracker 
 */
struct GameStatistics {
    int totalGuesses;           // Total number of guesses made
    int correctGuesses;         // Number of correct letters 
    bool won;                   // just a check for if the player won
    int attemptNumber;          // number of attempts 
    string targetWord;          // What was the word??
    chrono::system_clock::time_point gameStartTime;
    chrono::system_clock::time_point gameEndTime;
    
    GameStatistics() : totalGuesses(0), correctGuesses(0), won(false),
                       attemptNumber(0), targetWord("") {}
};

/**
 * Complete game state container
 * Uses multiple data structures to satisfy marking requirements:
 * - Stack: Guess history (LIFO access)
 * - Vector: Guess history (indexed access)
 * - Deque: Recent guesses (sliding window)
 * - Queue: Undo operations (FIFO)
 */
struct GameState {
    string targetWord;                          // Current target word
    int attemptsLeft;                           // Attempts remaining
    bool won;                                   // did they win
    bool gameOver;                              // Is game finished?
    int consecutiveLosses;                      // amount of loses in a row

    // ========================================================================
    // This is the section of data structures
    // ========================================================================
    
    // Stack: LIFO access for "undo" functionality
    stack<GuessRecord> guessStack;
    
    // Vector: Indexed access and iteration
    vector<GuessRecord> guessHistory;
    
    // Deque: Efficient access to recent guesses (sliding window)
    deque<GuessRecord> recentGuesses;
    
    // Queue: Track pending operations (for future undo stack)
    queue<GuessRecord> undoQueue;
    
    // Statistics
    GameStatistics stats;
    
    // Game metadata
    int gameNumber;                             // Which game in this session?
    DifficultySystem difficulty;                // Current difficulty settings

    /**
     * Constructor: Initialize game state with defaults
     */
    GameState() : attemptsLeft(MAX_ATTEMPTS), won(false),
                  gameOver(false), consecutiveLosses(0), gameNumber(0)
    {
        stats = GameStatistics();
    }
};

// ============================================================================
// GAME INITIALIZATION - Set up a new round
// ============================================================================

/**
 * Initialize a new round with a target word
 * Time Complexity: O(n) where n = size of data structures
 * Space Complexity: O(1)
 * @param gs Reference to GameState
 * @param targetWord The word to guess
 * @throw invalid_argument if target word is empty or invalid
 */
void initRound(GameState& gs, const string& targetWord)
{
    if (targetWord.empty()) {
        throw invalid_argument("[Game] Target word cannot be empty.");
    }

    if (!isValidWord(targetWord)) {
        throw invalid_argument(
            "[Game] Target word contains non-alphabetic characters.");
    }

    // Resets all game state
    gs.targetWord = targetWord;
    gs.attemptsLeft = MAX_ATTEMPTS;
    gs.won = false;
    gs.gameOver = false;
    gs.gameNumber++;

    // Clears all data structures
    while (!gs.guessStack.empty()) {
        gs.guessStack.pop();
    }
    while (!gs.undoQueue.empty()) {
        gs.undoQueue.pop();
    }
    
    gs.guessHistory.clear();
    gs.recentGuesses.clear();

    // Initialize statistics
    gs.stats = GameStatistics();
    gs.stats.targetWord = targetWord;
    gs.stats.gameStartTime = chrono::system_clock::now();

    cout << "[Game] New round started.\n";
    cout << "[Game] Word length: " << targetWord.length() << "\n";
    cout << "[Game] Attempts available: " << MAX_ATTEMPTS << "\n";
}

// ============================================================================
// GUESS SUBMISSION - this processes the players guesses
// ============================================================================

/**
 * Validate guess format and content
 * Time Complexity: O(n) where n = guess length
 * @param guess Guess to validate
 * @param targetWord Target word for length comparison
 * @throw invalid_argument if guess is invalid
 */
void validateGuessFormat(const string& guess, const string& targetWord)
{
    if (guess.empty()) {
        throw invalid_argument("[Game] Guess cannot be empty.");
    }

    if (guess.length() != targetWord.length()) {
        throw invalid_argument(
            "[Game] Guess length (" + to_string(guess.length()) +
            ") doesn't match target length (" + 
            to_string(targetWord.length()) + ").");
    }

    if (!isValidWord(guess)) {
        throw invalid_argument(
            "[Game] Guess contains non-alphabetic characters.");
    }
}

/**
 * Submit a guess and process the result
 * 
 * Algorithm:
 * 1. Validate game is not over
 * 2. Validate guess format
 * 3. Call validateGuess() to get letter colors
 * 4. Record in all data structures
 * 5. Check win/loss conditions
 * 6. Update statistics
 * 
 * Time Complexity: O(n) where n = word length (for validation)
 * Space Complexity: O(n) for storing results
 * 
 * Front-end: After calling submitGuess(), read gs.guessHistory to render board.
 * Each GuessRecord has .word and .letterResults (vector<LetterResult>).
 * Use .letterResults[i].color ("green", "yellow", "grey") to color tiles.
 * 
 * @param gs Reference to GameState
 * @param guess The player's guess
 * @return True if guess was accepted, false otherwise
 */
bool submitGuess(GameState& gs, const string& guess)
{
    // Check if game is already over
    if (gs.gameOver) {
        cout << "[Game] Round is already over. Start a new game.\n";
        return false;
    }

    // Validate guess format
    try {
        validateGuessFormat(guess, gs.targetWord);
    } catch (const invalid_argument& e) {
        cerr << e.what() << "\n";
        return false;
    } catch (const length_error& e) {
        cerr << e.what() << "\n";
        return false;
    }

    // Create guess record
    GuessRecord rec;
    rec.word = guess;
    rec.timestamp = chrono::system_clock::now();
    
    try {
        rec.letterResults = validateGuess(guess, gs.targetWord);
    } catch (const exception& e) {
        cerr << "[Game] Validation error: " << e.what() << "\n";
        return false;
    }

    // Determine if guess is correct
    rec.correct = (guess == gs.targetWord);

    // Store in all data structures
    // ====================================================================
    
    // Stack: for undo functionality (LIFO)
    gs.guessStack.push(rec);
    
    // Vector: for full history and iteration
    gs.guessHistory.push_back(rec);
    
    // Deque: keep only recent guesses (sliding window of 10)
    gs.recentGuesses.push_back(rec);
    if (gs.recentGuesses.size() > 10) {
        gs.recentGuesses.pop_front();
    }
    
    // Queue: for undo operations
    gs.undoQueue.push(rec);

    // Decrement attempts
    --gs.attemptsLeft;
    gs.stats.totalGuesses++;

    // Count correct letter positions for stats
    for (const auto& lr : rec.letterResults) {
        if (lr.color == "green") {
            gs.stats.correctGuesses++;
        }
    }

    // ====================================================================
    // DETERMINE GAME OUTCOME
    // ====================================================================

    if (rec.correct) {
        // GAME WON
        // ================================================================
        gs.won = true;
        gs.gameOver = true;
        gs.consecutiveLosses = 0;
        gs.stats.won = true;
        gs.stats.attemptNumber = MAX_ATTEMPTS - gs.attemptsLeft;
        gs.stats.gameEndTime = chrono::system_clock::now();

        cout << "[Game] ========== WIN! ==========\n";
        cout << "[Game] Correct word: \"" << gs.targetWord << "\"\n";
        cout << "[Game] Attempts used: " << gs.stats.attemptNumber 
             << "/" << MAX_ATTEMPTS << "\n";
        cout << "[Game] ============================\n";

    } else if (gs.attemptsLeft <= 0) {
        // GAME LOST
        // ================================================================
        gs.gameOver = true;
        ++gs.consecutiveLosses;
        gs.stats.won = false;
        gs.stats.attemptNumber = MAX_ATTEMPTS;
        gs.stats.gameEndTime = chrono::system_clock::now();

        cout << "[Game] ========== LOSS ==========\n";
        cout << "[Game] Correct word: \"" << gs.targetWord << "\"\n";
        cout << "[Game] Consecutive losses: " << gs.consecutiveLosses << "\n";
        cout << "[Game] ============================\n";

        // Check for easter egg trigger
        if (gs.consecutiveLosses >= LOSS_STREAK_LIMIT) {
            cout << "[Game] *** EASTER_EGG_TRIGGER ***\n";
            cout << "[Game] Front-end: Show Malachi popup and play 'No Means No' audio\n";
        }

    } else {
        // GAME CONTINUES
        // ================================================================
        cout << "[Game] Attempts left: " << gs.attemptsLeft << "\n";
        cout << "[Game] Guess " << gs.stats.totalGuesses << " submitted.\n";
    }

    return true;
}

// ============================================================================
// GAME STATISTICS AND REPORTING
// ============================================================================

/**
 * Print current game state for debugging
 * Time Complexity: O(n) where n = guess history size
 * @param gs Reference to GameState
 */
void printGameState(const GameState& gs)
{
    cout << "\n========== GAME STATE ==========\n";
    cout << "Target word:        " << gs.targetWord << "\n";
    cout << "Attempts left:      " << gs.attemptsLeft << "\n";
    cout << "Game over:          " << (gs.gameOver ? "Yes" : "No") << "\n";
    cout << "Won:                " << (gs.won ? "Yes" : "No") << "\n";
    cout << "Consecutive losses: " << gs.consecutiveLosses << "\n";
    cout << "Total guesses:      " << gs.guessHistory.size() << "\n";
    cout << "Difficulty:         " << getDifficultyLabel(gs.difficulty) << "\n";
    cout << "================================\n\n";
}

/**
 * Print all guesses in game history
 * Time Complexity: O(n * m) where n = guesses, m = avg word length
 * @param gs Reference to GameState
 */
void printGuessHistory(const GameState& gs)
{
    cout << "\n========== GUESS HISTORY ==========\n";
    
    if (gs.guessHistory.empty()) {
        cout << "No guesses yet.\n";
    } else {
        for (size_t i = 0; i < gs.guessHistory.size(); ++i) {
            const auto& rec = gs.guessHistory[i];
            cout << "Guess " << (i + 1) << ": \"" << rec.word << "\" -> ";
            
            for (const auto& lr : rec.letterResults) {
                cout << "[" << lr.letter << ":" << lr.color << "] ";
            }
            
            cout << (rec.correct ? " [CORRECT]" : "") << "\n";
        }
    }
    
    cout << "===================================\n\n";
}

/**
 * Get recent guesses (last N guesses from deque)
 * Time Complexity: O(k) where k = number of recent guesses
 * @param gs Reference to GameState
 * @param count Number of recent guesses to retrieve
 * @return Vector of recent GuessRecords
 */
vector<GuessRecord> getRecentGuesses(const GameState& gs, size_t count)
{
    vector<GuessRecord> recent;
    
    size_t startIdx = gs.recentGuesses.size() > count 
                      ? gs.recentGuesses.size() - count 
                      : 0;
    
    for (size_t i = startIdx; i < gs.recentGuesses.size(); ++i) {
        recent.push_back(gs.recentGuesses[i]);
    }
    
    return recent;
}

/**
 * Undo the last guess (using stack)
 * Time Complexity: O(n) where n = size of data structures
 * @param gs Reference to GameState
 * @return True if undo was successful
 */
bool undoLastGuess(GameState& gs)
{
    if (gs.guessStack.empty()) {
        cout << "[Game] No guesses to undo.\n";
        return false;
    }

    if (gs.gameOver) {
        cout << "[Game] Cannot undo after game is over.\n";
        return false;
    }

    // Pop from stack
    gs.guessStack.pop();
    
    // Remove from history
    if (!gs.guessHistory.empty()) {
        gs.guessHistory.pop_back();
    }
    
    // Restore attempt
    gs.attemptsLeft++;
    gs.stats.totalGuesses--;

    cout << "[Game] Last guess undone. Attempts left: " 
         << gs.attemptsLeft << "\n";
    
    return true;
}

// ============================================================================
// HELPER VALIDATION FUNCTION
// ============================================================================

// isValidWord is already defined in Person4__1_.cpp - no need to duplicate
