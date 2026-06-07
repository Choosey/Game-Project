#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <cctype>
#include <random>
#include <chrono>
#include <ctime>
#include <fstream>
#include <sstream>
#include <algorithm>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#endif

using namespace std;

const int MAX_ATTEMPTS = 6;

struct LetterResult {
    char letter;
    string color;
};

struct GuessRecord {
    string word;
    bool correct;
    vector<LetterResult> letterResults;
};

struct GameState {
    string targetWord;
    int attemptsLeft;
    bool won;
    bool gameOver;
    vector<GuessRecord> guessHistory;
};

map<string, GameState> activeGames;
vector<string> wordList;

vector<LetterResult> validateGuess(string guess, string targetWord) {
    vector<LetterResult> results;
    
    for (char& c : guess) c = tolower(c);
    for (char& c : targetWord) c = tolower(c);
    
    int len = targetWord.length();
    vector<bool> used(len, false);
    
    for (int i = 0; i < len; i++) {
        LetterResult r;
        r.letter = toupper(guess[i]);
        if (guess[i] == targetWord[i]) {
            r.color = "green";
            used[i] = true;
        } else {
            r.color = "gray";
        }
        results.push_back(r);
    }
    
    for (int i = 0; i < len; i++) {
        if (results[i].color == "green") continue;
        
        for (int j = 0; j < len; j++) {
            if (!used[j] && guess[i] == targetWord[j]) {
                results[i].color = "yellow";
                used[j] = true;
                break;
            }
        }
    }
    
    return results;
}

string readFile(const string& path) {
    ifstream file("public/" + path);
    if (!file.is_open()) return "";
    stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

string extractJsonValue(const string& json, const string& key) {
    string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == string::npos) return "";
    
    pos = json.find(":", pos);
    if (pos == string::npos) return "";
    
    pos++;
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
    
    if (json[pos] == '"') {
        pos++;
        size_t end = json.find("\"", pos);
        if (end != string::npos) return json.substr(pos, end - pos);
    } else {
        size_t end = json.find_first_of(",}", pos);
        if (end != string::npos) return json.substr(pos, end - pos);
    }
    
    return "";
}

string handleRequest(const string& method, const string& path, const string& body) {
    if (method == "GET") {
        if (path == "/" || path == "/index.html") {
            string content = readFile("index.html");
            if (!content.empty())
                return "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " + to_string(content.length()) + "\r\n\r\n" + content;
        }
        else if (path == "/UI.css") {
            string content = readFile("UI.css");
            if (!content.empty())
                return "HTTP/1.1 200 OK\r\nContent-Type: text/css\r\nContent-Length: " + to_string(content.length()) + "\r\n\r\n" + content;
        }
        else if (path == "/script.js") {
            string content = readFile("script.js");
            if (!content.empty())
                return "HTTP/1.1 200 OK\r\nContent-Type: application/javascript\r\nContent-Length: " + to_string(content.length()) + "\r\n\r\n" + content;
        }
        else if (path.find(".png") != string::npos) {
            ifstream file("public/" + path, ios::binary);
            if (file.is_open()) {
                string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
                return "HTTP/1.1 200 OK\r\nContent-Type: image/png\r\nContent-Length: " + to_string(content.length()) + "\r\n\r\n" + content;
            }
        }
        
        string response = "<html><body><h1>Wordle Server Running</h1></body></html>";
        return "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " + to_string(response.length()) + "\r\n\r\n" + response;
    }
    
    if (path == "/api/new_game" && method == "POST") {
        if (wordList.empty()) {
            wordList = {"APPLE", "GRAPE", "MANGO", "BERRY", "PEACH", "LEMON", 
                       "CHILI", "OLIVE", "GUAVA", "PLUMS", "BANJO", "CRANE", 
                       "SLATE", "BRICK", "STONE", "LIGHT", "NIGHT", "WATER", 
                       "FROST", "FLAME"};
        }
        
        srand(time(NULL));
        string targetWord = wordList[rand() % wordList.size()];
        string sessionId = to_string(rand()) + to_string(time(NULL));
        
        GameState gs;
        gs.targetWord = targetWord;
        gs.attemptsLeft = MAX_ATTEMPTS;
        gs.won = false;
        gs.gameOver = false;
        activeGames[sessionId] = gs;
        
        string response = "{\"sessionId\":\"" + sessionId + "\",\"wordLength\":" + 
                          to_string((int)targetWord.length()) + ",\"maxAttempts\":" + 
                          to_string(MAX_ATTEMPTS) + "}";
        
        return "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: " + 
               to_string(response.length()) + "\r\n\r\n" + response;
    }
    
    if (path == "/api/guess" && method == "POST") {
        string sessionId = extractJsonValue(body, "sessionId");
        string guess = extractJsonValue(body, "guess");
        
        for (char& c : guess) c = toupper(c);
        
        if (activeGames.find(sessionId) == activeGames.end()) {
            string response = "{\"success\":false,\"message\":\"Session not found\"}";
            return "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: " + 
                   to_string(response.length()) + "\r\n\r\n" + response;
        }
        
        GameState& gs = activeGames[sessionId];
        
        if (gs.gameOver) {
            string response = "{\"success\":false,\"message\":\"Game already over\"}";
            return "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: " + 
                   to_string(response.length()) + "\r\n\r\n" + response;
        }
        
        if (guess.length() != gs.targetWord.length()) {
            string response = "{\"success\":false,\"message\":\"Word must be " + to_string(gs.targetWord.length()) + " letters\"}";
            return "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: " + 
                   to_string(response.length()) + "\r\n\r\n" + response;
        }
        
        vector<LetterResult> results = validateGuess(guess, gs.targetWord);
        bool isCorrect = (guess == gs.targetWord);
        
        GuessRecord record;
        record.word = guess;
        record.correct = isCorrect;
        record.letterResults = results;
        gs.guessHistory.push_back(record);
        gs.attemptsLeft--;
        
        string resultsJson = "[";
        for (size_t i = 0; i < results.size(); i++) {
            if (i > 0) resultsJson += ",";
            resultsJson += "{\"letter\":\"" + string(1, results[i].letter) + "\",\"color\":\"" + results[i].color + "\"}";
        }
        resultsJson += "]";
        
        string response;
        
        if (isCorrect) {
            gs.won = true;
            gs.gameOver = true;
            response = "{\"success\":true,\"gameOver\":true,\"won\":true,\"guess\":{\"word\":\"" + guess + "\",\"letterResults\":" + resultsJson + "}}";
        } else if (gs.attemptsLeft <= 0) {
            gs.gameOver = true;
            response = "{\"success\":true,\"gameOver\":true,\"won\":false,\"targetWord\":\"" + gs.targetWord + "\",\"guess\":{\"word\":\"" + guess + "\",\"letterResults\":" + resultsJson + "}}";
        } else {
            response = "{\"success\":true,\"gameOver\":false,\"won\":false,\"attemptsLeft\":" + to_string(gs.attemptsLeft) + ",\"guess\":{\"word\":\"" + guess + "\",\"letterResults\":" + resultsJson + "}}";
        }
        
        return "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: " + 
               to_string(response.length()) + "\r\n\r\n" + response;
    }
    
    string response = "Not Found";
    return "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\nContent-Length: " + to_string(response.length()) + "\r\n\r\n" + response;
}

int main() {
    #ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    SOCKET server_fd;
    #else
    int server_fd;
    #endif
    
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int opt = 1;
    
    #ifdef _WIN32
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
    #else
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    #endif
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);
    
    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 3);
    
    cout << "\n========================================" << endl;
    cout << "  WORDLE GAME SERVER RUNNING" << endl;
    cout << "========================================" << endl;
    cout << "  Open: http://localhost:8080" << endl;
    cout << "========================================\n" << endl;
    
    while (true) {
        #ifdef _WIN32
        SOCKET client_fd = accept(server_fd, (struct sockaddr*)&address, &addrlen);
        #else
        int client_fd = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        #endif
        
        char buffer[32768] = {0};
        #ifdef _WIN32
        recv(client_fd, buffer, 32768, 0);
        #else
        read(client_fd, buffer, 32768);
        #endif
        
        string request(buffer);
        string method = request.substr(0, request.find(" "));
        string path = request.substr(request.find(" ") + 1);
        path = path.substr(0, path.find(" "));
        
        size_t bodyStart = request.find("\r\n\r\n");
        string body = (bodyStart != string::npos) ? request.substr(bodyStart + 4) : "";
        
        string response = handleRequest(method, path, body);
        
        #ifdef _WIN32
        send(client_fd, response.c_str(), response.length(), 0);
        closesocket(client_fd);
        #else
        write(client_fd, response.c_str(), response.length());
        close(client_fd);
        #endif
    }
    
    #ifdef _WIN32
    closesocket(server_fd);
    WSACleanup();
    #endif
    
    return 0;
}