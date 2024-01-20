#undef UNICODE

#define WIN32_LEAN_AND_MEAN
#include <iostream>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <vector>
#include <string>
#include <queue>
#include <sstream>
#include <stdio.h>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <unordered_set>
#include <future>
#include <memory>

using namespace std;
// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
   
#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

class Client {
public:
    SOCKET socket = INVALID_SOCKET;
    string name;
    int id;

    Client(SOCKET socket, int id) : socket(socket), id(id) {}
    Client(SOCKET socket, string name) : socket(socket), name(name) {}

    ~Client() {
        cout << "Removing Client " << socket << endl;
        closesocket(socket);
    }
};

// making a queue to store messages 

template<typename T>
class ConcurrentQueue {
    queue<T> m_data;
    mutex mtx;
    condition_variable cv;
public:
    void enqueue(const T& item) {
        unique_lock<mutex> lock(mtx);
        m_data.push(item);
        lock.unlock();
        cv.notify_one();
    }
    T dequeue(){
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [this] { return m_data.size() != 0; });
        T data = m_data.front();
        m_data.pop();
        return data;
    }
};

ConcurrentQueue< pair< string, Client* > > messageQueue;

class Server {
public:
    SOCKET ListenSocket = INVALID_SOCKET;
    vector<thread> clientThreads;
    int clientIdCounter = 0;
    unordered_set<Client*> connectedClients;
    // variable for server state
    atomic<bool> serverRunning{ true };

    Server() {
        if (initialize() != 0) {
            cout << "Error in starting the server. Retry starting again!" << endl;
            exit(1);
        }
        else {
            cout << "Server started successfully!!" << endl;
        }
    }

    ~Server() {
        serverRunning = false;
        closesocket(ListenSocket);

        // shutdown connected clients
        for (auto& client : connectedClients) {
            int iResult = shutdown(client->socket, SD_SEND);
            closesocket(client->socket);
            delete client;
        }

        // clean up client threads
        for (auto& thread : clientThreads) {
            if (thread.joinable()) thread.join();
        }

        connectedClients.clear();
        clientThreads.clear();
        WSACleanup();
    }

    // Handle a new client connection
    void handleNewClient(SOCKET clientSocket) {
        cout << "Client with socket id: " << clientSocket << " trying to enter the server." << endl;

        // Send a welcome message to the connected client
        string welcomeMessage = "Welcome to the Awesome Chat Server!\r";
        int sendResult = send(clientSocket, welcomeMessage.c_str(), welcomeMessage.size() + 1, 0);

        if (sendResult > 0) {
            Client* newClient = new Client(clientSocket, clientIdCounter++);
            ostringstream joinMessage;
            joinMessage << "SOCKET #" << clientSocket << ": joined the chat";
            string joinMessageStr = joinMessage.str();
            messageQueue.enqueue({ joinMessageStr, newClient });
            cout << "SOCKET #" << clientSocket << ": connected to the server\n";

            connectedClients.insert(newClient);
            clientThreads.push_back(thread(&Server::checkClientMessages, this, newClient));
        }
        else {
            cout << "client connection failed\n";
            closesocket(clientSocket);
        }
    }

    // accept client connections
    void acceptClientsLoop() {
        while (serverRunning) {
            SOCKET clientSocket = accept(ListenSocket, nullptr, nullptr);
            if (clientSocket != INVALID_SOCKET) {
                handleNewClient(clientSocket);
            }
        }
    }

    // check for messages from a specific client
    void checkClientMessages(Client* client) {
        int recvBufferSize = DEFAULT_BUFLEN;
        char recvBuffer[DEFAULT_BUFLEN];

        while (serverRunning) {
            int receiveResult = recv(client->socket, recvBuffer, recvBufferSize, 0);
            if (receiveResult > 0) {
                ostringstream messageStream;
                messageStream << "SOCKET #" << client->socket << ": " << recvBuffer << "\r";
                string messageStr = messageStream.str();
                cout << "Client with socket id: " << client->socket << " sent a message." << endl;
                messageQueue.enqueue({ messageStr, client });
            }
            else {
                cout << "Client with socket id: " << client->socket << " left." << endl;

                // Clean up disconnected client
                connectedClients.erase(client);
                delete client;
                return;
            }
        }
    }
private:
   
    int initialize() {
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            cout << "WSAStartup failed with error: " << result << endl;
            return 1;
        }

        struct addrinfo* resultInfo = nullptr;
        struct addrinfo hints;

        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        hints.ai_flags = AI_PASSIVE;

        result = getaddrinfo(nullptr, DEFAULT_PORT, &hints, &resultInfo);
        if (result != 0) {
            cout << "getaddrinfo failed with error: " << result << endl;
            WSACleanup();
            return 1;
        }

        ListenSocket = socket(resultInfo->ai_family, resultInfo->ai_socktype, resultInfo->ai_protocol);
        if (ListenSocket == INVALID_SOCKET) {
            cout << "socket failed with error: " << WSAGetLastError() << endl;
            freeaddrinfo(resultInfo);
            WSACleanup();
            return 1;
        }

        result = ::bind(ListenSocket, resultInfo->ai_addr, static_cast<int>(resultInfo->ai_addrlen));
        if (result == SOCKET_ERROR) {
            cout << "bind failed with error: " << WSAGetLastError() << endl;
            freeaddrinfo(resultInfo);
            closesocket(ListenSocket);
            WSACleanup();
            return 1;
        }

        freeaddrinfo(resultInfo);
        result = listen(ListenSocket, SOMAXCONN);
        if (result == SOCKET_ERROR) {
            cout << "listen failed with error: " << WSAGetLastError() << endl;
            closesocket(ListenSocket);
            WSACleanup();
            return 1;
        }

        return 0;
    }
};

// function to send messages from the messageQueue to clients
void sendMessage(Server* server) {
    while (server->serverRunning) {
        pair<string, Client*> messageItem = messageQueue.dequeue();

        vector<future<void>> futures;
        for (auto& client : server->connectedClients) {
            auto future = async(launch::async, [server , &client, &messageItem] {
                if (client != messageItem.second) {
                    int sendResult = send(client->socket, messageItem.first.c_str(), messageItem.first.length() + 1, 0);
                    if (sendResult == SOCKET_ERROR) {
                        cout << "send failed with error: " << WSAGetLastError() << endl;

                        server->connectedClients.erase(client);
                        delete client;
                    }
                }
                });

            futures.push_back(move(future));
        }

        for_each(futures.begin(), futures.end(), [](future<void>& future) { future.wait(); });
        cout << "Message has been sent to all clients." << endl;
    }
}

int __cdecl main(void)
{
    unique_ptr<Server> server = make_unique<Server>();
    // thread for accepting client connections
    thread acceptClientThread(&Server::acceptClientsLoop, server.get());

    // thread for sending messages to clients
    thread sendMessageThread(sendMessage, server.get());

    if (acceptClientThread.joinable()) acceptClientThread.join();
    if (sendMessageThread.joinable()) sendMessageThread.join();
    return 0;
}