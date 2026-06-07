//Keyon Bertrand
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

#include "gamecore.cpp"
#include "Person_3.cpp"
#include "Person_4.cpp"
#include "word_system.cpp"

map<string, GameState> activeGames;
vector<string> wordList;

string readFile(const string& path) {
    ifstream file("public/" + path);
    if (!file.is_open()) return "";
    stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

string getMimeType(const string& path) {
    if (path.find(".html") != string::npos) return "text/html";
    if (path.find(".css") != string::npos) return "text/css";
    if (path.find(".js") != string::npos) return "application/javascript";
    if (path.find(".png") != string::npos) return "image/png";
    return "text/plain";
}

string extractJsonValue(const string& json, const string& key) {
    string searchKey = "\"" + key + "\"";
    size_t keyPos = json.find(searchKey);
    if (keyPos == string::npos) return "";
    size_t colonPos = json.find(":", keyPos);
    if (colonPos == string::npos) return "";
    size_t valueStart = colonPos + 1;
    while (valueStart < json.length() && (json[valueStart] == ' ' || json[valueStart] == '\t')) valueStart++;
    if (json[valueStart] == '"') {
        valueStart++;
        size_t valueEnd = json.find("\"", valueStart);
        if (valueEnd != string::npos) return json.substr(valueStart, valueEnd - valueStart);
    } else {
        size_t valueEnd = json.find_first_of(",}", valueStart);
        if (valueEnd != string::npos) return json.substr(valueStart, valueEnd - valueStart);
    }
    return "";
}

string handleRequest(const string& method, const string& path, const string& body) {
    if (method == "GET") {
        if (path == "/") {
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
        initRound(gs, targetWord);
        activeGames[sessionId] = gs;
        
        string response = "{\"sessionId\":\"" + sessionId + "\",\"wordLength\":" + 
                          to_string(targetWord.length()) + ",\"maxAttempts\":6}";
        
        return "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: " + 
               to_string(response.length()) + "\r\n\r\n" + response;
    }
    
    if (path == "/api/guess" && method == "POST") {
        string sessionId = extractJsonValue(body, "sessionId");
        string guess = extractJsonValue(body, "guess");
        
        for (char& c : guess) c = toupper(c);
        
        if (activeGames.count(sessionId)) {
            GameState& gs = activeGames[sessionId];
            
            bool accepted = submitGuess(gs, guess);
            
            if (accepted && !gs.guessHistory.empty()) {
                GuessRecord lastGuess = gs.guessHistory.back();
                
                string resultsJson = "[";
                for (size_t i = 0; i < lastGuess.letterResults.size(); i++) {
                    if (i > 0) resultsJson += ",";
                    resultsJson += "{\"letter\":\"" + string(1, lastGuess.letterResults[i].letter) + 
                                   "\",\"color\":\"" + lastGuess.letterResults[i].color + "\"}";
                }
                resultsJson += "]";
                
                string response;
                if (gs.won) {
                    response = "{\"success\":true,\"gameOver\":true,\"won\":true,\"guess\":{\"word\":\"" + 
                               lastGuess.word + "\",\"letterResults\":" + resultsJson + "},\"message\":\"You won!\"}";
                } else if (gs.gameOver) {
                    response = "{\"success\":true,\"gameOver\":true,\"won\":false,\"targetWord\":\"" + 
                               gs.targetWord + "\",\"message\":\"Game over!\"}";
                } else {
                    response = "{\"success\":true,\"gameOver\":false,\"won\":false,\"attemptsLeft\":" + 
                               to_string(gs.attemptsLeft) + ",\"guess\":{\"word\":\"" + lastGuess.word + 
                               "\",\"letterResults\":" + resultsJson + "}}";
                }
                return "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: " + 
                       to_string(response.length()) + "\r\n\r\n" + response;
            } else {
                string response = "{\"success\":false,\"message\":\"Invalid guess\"}";
                return "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: " + 
                       to_string(response.length()) + "\r\n\r\n" + response;
            }
        }
    }
    
    string response = "<html><body><h1>404 Not Found</h1></body></html>";
    return "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\nContent-Length: " + 
           to_string(response.length()) + "\r\n\r\n" + response;
}

int main() {
    #ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    #endif
    
    #ifdef _WIN32
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
    
    cout << "========================================" << endl;
    cout << "Wordle Game Server Running!" << endl;
    cout << "Open browser to: http://localhost:8080" << endl;
    cout << "========================================" << endl;
    
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
    #else
    close(server_fd);
    #endif
    
    return 0;
}