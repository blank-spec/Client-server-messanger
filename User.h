#pragma once
#include <WinSock2.h>
#include <string>
#include <unordered_set>
#include <unordered_map>
using namespace std;

struct User {
    SOCKET                                      userSocket;
    string                                      name;
    bool                                        isActive;
    bool                                        isInChat;
    long long                                   userID;
    long long                                   currentChat = -1;
    bool                                        isBanned;
    unordered_set<shared_ptr<User>>             chatIDs;
    unordered_set<long long>                    friends;
    vector<string>                              privateUnreadMessages;
    unordered_map<long long, vector<string>>    unreachedMessages;

    User(string& name, long long ID, SOCKET socket) :
        name(name),
        userID(ID),
        userSocket(socket),
        isActive(false),
        isInChat(false),
        isBanned(false) {}


    bool operator==(const shared_ptr<User>& other) const {
        return other->userID == userID;
    }

    void readUnreadPrivateMessages() {
        for (auto it = privateUnreadMessages.begin(); it != privateUnreadMessages.end(); ++it) {
            send(userSocket, (*it).c_str(), (*it).size(), 0);
            privateUnreadMessages.erase(it);
        }
    }

    void printID() const {
        send(userSocket, to_string(userID).c_str(), to_string(userID).size(), 0);
    }

    void addFriend(shared_ptr<User>& user) {
        friends.insert(user->userID);
        const string message = name + " add you to the friends. What about you?";
        send(user->userSocket, message.c_str(), message.size(), 0);
    }

    void sendToFriend(shared_ptr<User>& user, const string& message) {
        if (user->friends.find(userID) != user->friends.end()) {
            if (user->isInChat) {
                const string notify = "New message from your friend!";
                send(user->userSocket, notify.c_str(), notify.size(), 0);
                privateUnreadMessages.push_back(message);
            }
            send(user->userSocket, message.c_str(), message.size(), 0);
        }
    }
};
