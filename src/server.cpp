#include "server.hpp"
// function to start server

#include <unordered_map>

#include <string>

struct User {
    std::string username;
    int age;

    // Default constructor
    User() = default;

    // Custom constructor
    User(const std::string& uname, int a) : username(uname), age(a) {}

    // Copy constructor
    User(const User& other) = default;

    // Assignment operator
    User& operator=(const User& other) = default;
};


std::unordered_map<std::string, User> fakeDB = {
    { "camden", { "camden", 18 } },
    { "alex", { "alex", 22 } },
    { "devon", { "devon", 30 } }
};


int main() {
    start_server();  // Function we'll define in server.cpp
    return 0;
}
// guesing this is basically your import statements
#include "server.hpp"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
// defining port to 8080
#define PORT 8080
// function to start server setting types and the size of the memory address?
void start_server() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[30000] = {0};

    // this creates and try catches the web socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Socket failed\n";
        return;
    }

    // Attach socket to the port
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed\n";
        return;
    }

    // Listen for server
    listen(server_fd, 3);
    std::cout << "Listening on port " << PORT << "...\n";

    // Accept the web socket
    new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
    read(new_socket, buffer, 30000);
    std::cout << "Received request:\n" << buffer << "\n";

    if (strstr(buffer, "POST /login")) {
        // Step 1: Find where the headers end (two newlines = body start)
        char *body = strstr(buffer, "\r\n\r\n");
        if (body != NULL) {
            body += 4; // skip past the \r\n\r\n
            std::string bodyStr(body);
            std::cout << "Parsed body: " << bodyStr << "\n";

            // Step 2: Simple parsing (you can improve later)
            std::string username;
            std::string password;

            size_t userPos = bodyStr.find("username=");
            size_t passPos = bodyStr.find("password=");
            if (userPos != std::string::npos && passPos != std::string::npos) {
                username = bodyStr.substr(userPos + 9, passPos - userPos - 10); // crude extraction
                password = bodyStr.substr(passPos + 9);
            }

            std::string responseBody = "Welcome, " + username + "!";

            std::string response =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: " + std::to_string(responseBody.size()) + "\r\n\r\n" +
                responseBody;

            write(new_socket, response.c_str(), response.size());
        } else {
            // No body found
            const char *bad = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
            write(new_socket, bad, strlen(bad));
        }
    } else if (strstr(buffer, "GET /user/")) {
        // Extract the username from the URL
        std::string request(buffer);
        size_t start = request.find("GET /user/") + 10;
        size_t end = request.find(" ", start); // End of URL path
        std::string username = request.substr(start, end - start);

        std::cout << "Looking up user: " << username << "\n";

        // Check if user exists
        if (fakeDB.find(username) != fakeDB.end()) {
            User u = fakeDB[username];
            std::string json = "{ \"username\": \"" + u.username + "\", \"age\": " + std::to_string(u.age) + " }";

            std::string response =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: application/json\r\n"
                "Content-Length: " + std::to_string(json.size()) + "\r\n\r\n" +
                json;

            write(new_socket, response.c_str(), response.size());
        } else {
            const char *notfound = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
            write(new_socket, notfound, strlen(notfound));
        }

    } else {
        const char *bad = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
        write(new_socket, bad, strlen(bad));
    }

    close(new_socket);
}

