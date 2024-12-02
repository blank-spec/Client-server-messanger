#pragma once
#include <unordered_map>
#include "Chat.h"
#include "User.h"
using namespace std;

// Класс ChatManager управляет чатами и взаимодействием пользователей в них.
// Он хранит информацию о чатах и позволяет пользователям создавать, подключаться и выходить из чатов.

class ChatManager {
private:
    // Хранит активные чаты, индексируемые по уникальному идентификатору.
    unordered_map<long long, Chat> chats;

    vector<long long> unusedIDs;

    // Следующий доступный уникальный идентификатор чата.
    long long nextChatID = 1;

    // Метод уведомляет всех пользователей в чате о событии.
    // Параметры:
    // message - сообщение для уведомления.
    // chatID - идентификатор чата, в котором нужно отправить уведомление.
    void notify(const std::string& message, long long chatID) {
        auto it = chats.find(chatID);

        for (const auto& c : it->second.usersInChat) {
            send(c->userSocket, message.c_str(), message.size(), 0);
        }
    }

    // Метод отправляет сообщение конкретному пользователю.
    // Параметры:
    // socket - сокет пользователя, которому отправляется сообщение.
    // message - сообщение для отправки.
    void notification(SOCKET socket, const string& message) {
        send(socket, message.c_str(), message.size(), 0);
    }

public:

    // Метод отправляет сообщение всем пользователям в чате.
    // Параметры:
    // user - указатель на пользователя, от имени которого отправляется сообщение.
    // chatID - идентификатор чата.
    // message - само сообщение.
    void sendToUsers(shared_ptr<User> user, int chatID, const string& message) {
        auto chat = chats.find(chatID);
        chat->second.sendToUsers(user, message);
    }

    // Метод создает новый чат.
    // Параметры:
    // user - указатель на пользователя, создающего чат.
    // maxUsers - максимальное количество пользователей в чате (по умолчанию INT_MAX).
    void createChat(shared_ptr<User> user, int maxUsers = INT_MAX) {
        long long chatID;

        if (!unusedIDs.empty()) {
            chatID = *unusedIDs.begin();
            unusedIDs.erase(unusedIDs.begin());
        }
        else {
            chatID = nextChatID++;
        }

        Chat chat(maxUsers, chatID);
        chats[chatID] = chat;

        user->currentChat = chatID;
        user->isInChat = true;

        chats[chatID].usersInChat.push_back(user);
        chat.countUsers++;

        chat.moderators.insert(user);
        chat.creator = user;
    }

    // Метод подключает пользователя к чату.
    // Параметры:
    // user - указатель на пользователя, который подключается к чату.
    // chatID - идентификатор чата, к которому пользователь хочет подключиться.
    void connectToTheChat(shared_ptr<User> user, long long chatID) {
        auto chat = chats.find(chatID);

        if (chat != chats.end()) {
            if (chat->second.countUsers >= chat->second.maxUsers) {
                std::string message = "Chat is full";
                send(user->userSocket, message.c_str(), message.size(), 0);
                return;
            }

            user->isInChat = true;
            user->currentChat = chatID;
            user->chatIDs.insert(user);

            chat->second.usersInChat.push_back(user);

            notify(user->name + " has joined the chat", chatID);
            chat->second.countUsers++;
        }
        else {
            const std::string message = "Chat not found";
            send(user->userSocket, message.c_str(), message.size(), 0);
        }
    }

    void deleteChat(shared_ptr<User> user) {
        auto chat = chats.find(user->currentChat);
        if (chat->second.creator == user) {
            for (auto& users : chat->second.usersInChat)
                leaveChat(user);
            unusedIDs.push_back(chat->second.chatID);
            chats.erase(chat->second.chatID);
        }
        else {
            const string message = "Command not found";
            send(user->userSocket, message.c_str(), message.size(), 0);
        }
    }

    // Метод позволяет пользователю выйти из чата.
    // Параметры:
    // user - указатель на пользователя, который выходит из чата.
    void leaveChat(shared_ptr<User> user) {
        auto chat = chats.find(user->currentChat);
        if (chat != chats.end()) {
            auto it = find_if(chat->second.usersInChat.begin(), chat->second.usersInChat.end(), [&user](shared_ptr<User> u) {
                return user == u;
                });

            if (it != chat->second.usersInChat.end()) {
                chat->second.usersInChat.erase(it);
                chat->second.countUsers--;

                if (chat->second.moderators.find(user) != chat->second.moderators.end()) {
                    chat->second.moderators.erase(user);
                }

                notify(user->name + " has left the chat", user->currentChat);
                user->chatIDs.erase(user);
                user->currentChat = -1;
                user->isInChat = false;
            }
        }
    }

    void banUser(shared_ptr<User> moderator, shared_ptr<User> toBan) {
        auto chat = chats.find(moderator->currentChat);
        if (chat->second.moderators.find(moderator) != chat->second.moderators.end()) {
            chat->second.blacklist.insert(toBan->userID);
            const string message = "You have banned in this chat";
            send(toBan->userSocket, message.c_str(), message.size(), 0);
        }
        else {
            const string message = "You need to be moderator, for banning users";
            send(moderator->userSocket, message.c_str(), message.size(), 0);
        }
    }

    void removeBan(shared_ptr<User> moderator, shared_ptr<User> banned) {
        auto chat = chats.find(moderator->currentChat);
        if (chat->second.moderators.find(moderator) != chat->second.moderators.end()) {
            chat->second.blacklist.erase(banned->userID);
            const string message = "Your bun has been removed";
            send(banned->userSocket, message.c_str(), message.size(), 0);
        }
        else {
            const string message = "Command not found";
            send(moderator->userSocket, message.c_str(), message.size(), 0);
        }
    }
};
