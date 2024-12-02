#include <WinSock2.h>
#include <iostream>
#include <mutex>
#include <thread>
#include <ws2tcpip.h>
#include "User.h"
#include <sstream>
#include "ChatManager.h"
#include <unordered_map>
#include <algorithm>

#pragma comment(lib, "Ws2_32.lib")
using namespace std;

class Server {
private:
    unordered_map<long long, shared_ptr<User>>      users;
    ChatManager                                     manager;
    long long                                       id = 1;
    mutex                                           mtx;

    void handleClient(SOCKET clientSocket) {
        string strID = to_string(id);
        auto user = std::make_shared<User>(strID, id, clientSocket);
        ++id;
        user->isActive = true;
        users.insert({ user->userID, user });
        char buffer[1024];

        while (true) {
            int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

            if (bytesReceived <= 0) {
                std::cout << "Client disconnected." << std::endl;
                closesocket(clientSocket);
                return;
            }

            std::string message = std::string(buffer, 0, bytesReceived);

            if (message.starts_with("/create")) {
                std::lock_guard<std::mutex> lock(mtx);
                manager.createChat(user);
                continue;
            }
            else if (message.starts_with("/showid")) {
                std::lock_guard<std::mutex> lock(mtx);
                user->printID();
                continue;
            }
            else if (message.starts_with("/connect ")) {
                std::lock_guard<std::mutex> lock(mtx);
                int connectID = stoi(message.substr(8));
                cout << id << endl;
                manager.connectToTheChat(user, connectID);
                continue;
            }
            else if (message.starts_with("/leave")) {
                std::lock_guard<std::mutex> lock(mtx);
                manager.leaveChat(user);
                continue;
            }
            else {
                std::lock_guard<std::mutex> lock(mtx);
                if (user->isInChat)
                    manager.sendToUsers(user, user->currentChat, message);
                continue;
            }
        }
    }

public:
    int run(const std::string& ip) {
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            std::cerr << "WSAStartup failed: " << result << std::endl;
            return 1;
        }

        SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (serverSocket == INVALID_SOCKET) {
            std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
            WSACleanup();
            return 1;
        }

        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(8080);
        const char* ipAddress = ip.c_str();
        if (inet_pton(AF_INET, ipAddress, &serverAddr.sin_addr) <= 0) {
            std::cerr << "Invalid address: " << ipAddress << std::endl;
            closesocket(serverSocket);
            WSACleanup();
            return 1;
        }

        if (bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
            closesocket(serverSocket);
            WSACleanup();
            return 1;
        }

        if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
            std::cerr << "Listen failed: " << WSAGetLastError() << std::endl;
            closesocket(serverSocket);
            WSACleanup();
            return 1;
        }

        std::cout << "Server is running. Waiting for clients..." << std::endl;

        while (true) {
            SOCKET clientSocket = accept(serverSocket, NULL, NULL);
            if (clientSocket == INVALID_SOCKET) {
                std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
                closesocket(serverSocket);
                WSACleanup();
                return 1;
            }

            std::cout << "Client connected!" << std::endl;
            std::thread clientThread(&Server::handleClient, this, clientSocket);
            clientThread.detach();
        }

        closesocket(serverSocket);
        WSACleanup();
        return 0;
    }
};
