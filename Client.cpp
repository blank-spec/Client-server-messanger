#include <WinSock2.h>
#include <ws2tcpip.h>

#include <fstream>
#include <iostream>
#include <string>
#include <thread>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

string createName() {
  ifstream ifile("Server/datas.txt");
  string line;
  if (getline(ifile, line)) {
    return line;
  }

  ofstream ofile("Server/datas.txt");
  string newName;
  cout << "Create new name: ";
  getline(cin, newName);
  ofile << newName;
  return newName;
}

void receiveMessages(SOCKET serverSocket) {
  char buffer[1024];
  while (true) {
    int bytesReceived = recv(serverSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived > 0) {
      std::string message(buffer, 0, bytesReceived);
      std::cout << message << std::endl
                << endl;
    } else if (bytesReceived == 0) {
      std::cout << "Server closed connection." << std::endl;
      break;
    } else {
      std::cerr << "recv failed: " << WSAGetLastError() << std::endl;
      break;
    }
  }
}

int main() {
  string name = createName();

  WSADATA wsaData;
  int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (result != 0) {
    std::cerr << "WSAStartup failed: " << result << std::endl;
    return 1;
  }

  SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (clientSocket == INVALID_SOCKET) {
    std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
    WSACleanup();
    return 1;
  }

  sockaddr_in serverAddr;
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(8080);

  const char* serverIp = ""; // Enter servers IP
  if (inet_pton(AF_INET, serverIp, &serverAddr.sin_addr) <= 0) {
    std::cerr << "Invalid address: " << serverIp << std::endl;
    closesocket(clientSocket);
    WSACleanup();
    return 1;
  }

  result = connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
  if (result == SOCKET_ERROR) {
    std::cerr << "Connection failed: " << WSAGetLastError() << std::endl;
    closesocket(clientSocket);
    WSACleanup();
    return 1;
  }

  std::cout << "Connected to the server." << std::endl;

  std::string chatID;

  // Создаем поток для получения сообщений от сервера
  std::thread receiveThread(receiveMessages, clientSocket);
  receiveThread.detach();

  // Основной поток для отправки сообщений на сервер
  std::string message;
  while (true) {
    std::getline(std::cin, message);
    if (message == "/quit") {
      break;
    } else if (message.starts_with("/join ")) {
      std::string newChatID = message.substr(6);
      send(clientSocket, message.c_str(), message.size(), 0);
      chatID = newChatID;
      continue;
    } else if (message.starts_with("/change ")) {
      string newChatID = message.substr(8);
      send(clientSocket, message.c_str(), message.size(), 0);
      chatID = newChatID;
      continue;
    } else if (message.starts_with("/make ")) {
      send(clientSocket, message.c_str(), message.size(), 0);
      continue;
    } else if (message.starts_with("/rename")) {
      send(clientSocket, message.c_str(), message.size(), 0);
      continue;
    }

    string formName = name + ": " + message;
    if (!message.empty()) {
      int sendResult = send(clientSocket, formName.c_str(), formName.size(), 0);
      if (sendResult == SOCKET_ERROR) {
        std::cerr << "send failed: " << WSAGetLastError() << std::endl;
        break;
      }
    }
  }

  closesocket(clientSocket);
  WSACleanup();
  return 0;
}
