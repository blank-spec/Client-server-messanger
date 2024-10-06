#include <WinSock2.h>
#include <ws2tcpip.h>

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <iostream>
#include <mutex>
#include <ostream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#pragma comment(lib, "Ws2_32.lib")

std::unordered_map<std::string, std::vector<SOCKET>> chatRooms;  // Чаты с клиентами
std::mutex chatRoomsMutex;

using namespace std;

void loadChatID() {
  ifstream file("C:/Game Downloads/BankProject/Server/IDES.txt");
  string ID;
  while (getline(file, ID)) {
    chatRooms[ID];
  }
  file.close();
}

void changeChatIDInFile(string& oldID, string& newID) {
  const string path = "C:/Game Downloads/BankProject/Server/IDES.txt";
  vector<string> ides;
  string line;

  ifstream inputFile(path);

  while (getline(inputFile, line)) {
    ides.push_back(line);
  }
  inputFile.close();

  for (auto& id : ides) {
    if (id == oldID) {
      id = newID;
    }
  }

  ofstream file(path);
  for (const auto& l : ides) {
    file << l << endl;
  }
  file.close();
}

std::string getDate() {
  auto end = std::chrono::system_clock::now();
  std::time_t end_time = std::chrono::system_clock::to_time_t(end);
  std::string dateStr = std::ctime(&end_time);
  dateStr.erase(dateStr.length() - 1);  // Удаляем символ новой строки
  return "[" + dateStr + "]";
}

void saveMessageToFile(const std::string& message, const std::string& chatID) {
  std::ofstream outFile("Server/" + chatID + "_messages.txt", std::ios::app);  // Сохраняем историю для каждого чата
  if (outFile.is_open()) {
    outFile << message << std::endl;
    outFile.close();
  } else {
    std::cerr << "Error opening file for saving message!" << std::endl;
  }
}

void sendHistoryToClient(SOCKET clientSocket, const std::string& chatID) {
  std::ifstream inFile("Server/" + chatID + "_messages.txt");
  std::string line;
  if (inFile.is_open()) {
    while (getline(inFile, line)) {
      send(clientSocket, line.c_str(), line.size(), 0);
      send(clientSocket, "\n", 1, 0);
    }
    inFile.close();
  } else {
    std::cerr << "Error opening file for reading message history!" << std::endl;
  }
}

void saveChatToFile(const string& chatID) {
  ofstream file("Server/Chats.txt", ios::app);
  file << chatID + ": " + "Server/" + chatID + "_messages.txt" << endl;
  file.close();
}

void handleClient(SOCKET clientSocket) {
  char buffer[1024];
  std::string chatID;

  while (true) {
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived <= 0) {
      std::cout << "Client disconnected." << std::endl;
      closesocket(clientSocket);
      std::lock_guard<std::mutex> lock(chatRoomsMutex);
      if (!chatID.empty()) {
        auto& clients = chatRooms[chatID];
        clients.erase(std::remove(clients.begin(), clients.end(), clientSocket), clients.end());
      }
      return;
    }

    std::string message = std::string(buffer, 0, bytesReceived);
    if (message.find("/make") == string::npos && message.find("/join") == string::npos && message.find("/change") == string::npos && message.find("/rename") == string::npos) {  // Проверяем, начинается ли сообщение с "/"
      saveMessageToFile(getDate() + message, chatID);
    }

    cout << message << endl;

    if (message.starts_with("/")) {
      if (message.starts_with("/make ")) {
        chatID = message.substr(6);
        if (chatID.empty()) {
          std::cerr << "Chat ID cannot be empty!" << std::endl;
          closesocket(clientSocket);
          return;
        }

        if (chatRooms.find(chatID) != chatRooms.end()) {
          string formedMess = format("The chat with id {} already exists", chatID);
          send(clientSocket, formedMess.c_str(), formedMess.size(), 0);
          continue;
        }
        // saveChatToFile(chatID);

        std::lock_guard<std::mutex> lock(chatRoomsMutex);
        chatRooms[chatID].push_back(clientSocket);
        [&chatID]() {
          ofstream file("Server/IDES.txt", ios::app);
          file << chatID << endl;
          file.close();
        }();
        // saveChatToFile(chatID);
        //  sendHistoryToClient(clientSocket, chatID);
        continue;
      } else if (message.starts_with("/join ")) {
        std::string newChatID = message.substr(6);
        if (newChatID.empty() || chatRooms.find(newChatID) == chatRooms.end()) {
          string mess = format("Chat room with id {} not found", newChatID);
          send(clientSocket, mess.c_str(), mess.size(), 0);
          continue;
        }

        {
          std::lock_guard<std::mutex> lock(chatRoomsMutex);
          auto& clients = chatRooms[chatID];
          clients.erase(std::remove(clients.begin(), clients.end(), clientSocket), clients.end());
          chatRooms[newChatID].push_back(clientSocket);
          chatID = newChatID;
          string reciveMess = format("You have joined to the chat room {}", newChatID);
          send(clientSocket, reciveMess.c_str(), reciveMess.size(), 0);
          sendHistoryToClient(clientSocket, chatID);
        }
        continue;
      } else if (message.starts_with("/change ")) {
        string newID = message.substr(8);
        {
          lock_guard<mutex> lock(chatRoomsMutex);  // Блокируем доступ к chatRooms
          if (chatRooms.find(newID) == chatRooms.end()) {
            string mess = format("Chat with id {} doesn't exist", newID);
            send(clientSocket, mess.c_str(), mess.size(), 0);
            continue;  // Прерываем выполнение, если чата с таким ID нет
          }
          auto& clients = chatRooms[chatID];
          clients.erase(std::remove(clients.begin(), clients.end(), clientSocket), clients.end());
          chatRooms[newID].push_back(clientSocket);
          chatID = newID;
          string reciveMess = format("You have changed your chat room to {}", newID);
          send(clientSocket, reciveMess.c_str(), reciveMess.size(), 0);
        }
        continue;
      } else if (message.starts_with("/rename ")) {
        if (chatID.empty()) {
          string mess = "You dont connect to any chat";
          send(clientSocket, mess.c_str(), mess.size(), 0);
          continue;
        }

        std::string newChatID = message.substr(8);
        std::string oldName = "Server/" + chatID + "_messages.txt";
        std::string newName = "Server/" + newChatID + "_messages.txt";

        changeChatIDInFile(chatID, newChatID);
        try {
          std::filesystem::rename(oldName, newName);
        } catch (const std::filesystem::filesystem_error& e) {
          std::cout << e.what() << std::endl;
        }

        {
          std::lock_guard<std::mutex> lock(chatRoomsMutex);
          if (chatRooms.find(newChatID) == chatRooms.end()) {
            chatRooms[newChatID] = {};
          }

          auto& oldSockets = chatRooms[chatID];
          chatRooms[newChatID].insert(chatRooms[newChatID].end(), oldSockets.begin(), oldSockets.end());
          oldSockets.clear();
          chatRooms.erase(chatID);
          chatID = newChatID;
          string mess = format("You have changed chat id to {}", newChatID);
          send(clientSocket, mess.c_str(), mess.size(), 0);
        }
        continue;
      }
    }
    std::lock_guard<std::mutex> lock(chatRoomsMutex);
    for (SOCKET client : chatRooms[chatID]) {
      if (client != clientSocket) {
        string formMess = getDate() + message;
        send(client, formMess.c_str(), formMess.size(), 0);
      }
    }
  }
}

int main() {
  loadChatID();
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
  const char* ipAddress = ""; // Enter your IP
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
    std::thread clientThread(handleClient, clientSocket);
    clientThread.detach();
  }

  closesocket(serverSocket);
  WSACleanup();
  return 0;
}
