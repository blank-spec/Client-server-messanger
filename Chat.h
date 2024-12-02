#pragma once
#include <vector>
#include <format>
#include <unordered_set>

struct User;

struct Chat {
    shared_ptr<User>                        creator;
    std::vector<shared_ptr<User>>           usersInChat;
    vector<string>                          messages;
    std::unordered_set<shared_ptr<User>>    moderators;
    std::unordered_set<long long>           blacklist;
    int                                     countUsers = 0;
    int                                     maxUsers;
    long long                               chatID;

    Chat(int maxUsers = INT_MAX, long long id = 0)
        : maxUsers(maxUsers), chatID(id) {
    }

    void sendToUsers(shared_ptr<User> user, const string& message) {
        if (blacklist.find(user->userID) != blacklist.end()) {
            send(user->userSocket, (const char*)"You banned in this chat", 24, 0);
            return;
        }

        for (auto& chatUser : usersInChat) {
            if (chatUser->userSocket == user->userSocket) 
                continue;

            if (chatUser->currentChat != chatID) 
                chatUser->unreachedMessages[chatID].emplace_back(message);
            else 
                send(chatUser->userSocket, message.c_str(), message.size(), 0);
        }
    }

    void addUser(shared_ptr<User>& user) {
        if (countUsers >= maxUsers) {
            return;
        }

        usersInChat.push_back(user);
        ++countUsers;
    }
};
