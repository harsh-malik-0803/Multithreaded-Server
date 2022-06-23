#undef UNICODE

#define WIN32_LEAN_AND_MEAN
#include <iostream>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <map>
#include <vector>
#include <string>
#include <queue>
#include <sstream>
#include <stdio.h>
using namespace std;
// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")
   
#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

class semaphore {
    int s;
public:
    semaphore() {
        s = 1;
    }
    void wait() {
        while (s <= 0);
        ++s;
    }
    void signal() {
        --s;
    }
};

semaphore sem1, sem2;

// client class 
class client {
public:
    SOCKET ClientSocket;
    string name;
    client() {
        ClientSocket = INVALID_SOCKET;
        name = "";
    }
    client(SOCKET s1) {
        ClientSocket = s1;
    }
    client(SOCKET s1, string n) {
        ClientSocket = s1;
        name = n;
    }
    ~client() {
        cout << "Removing client " << ClientSocket << endl;
        // cleanup
        closesocket(ClientSocket);
        WSACleanup();
    }
};

class queue_type {
public:
    string message;
    client* newclient;
    queue_type() {
        message = "";
        newclient = NULL;
    }
    queue_type(string mes, client* c) {
        message = mes;
        newclient = c;
    }
};
// making a queue to store messages 
queue<queue_type> message_q;
bool server_running = true;
map< client*, int > clients;

// Define a Lambda Expression for checking message arrived or not 
auto func2 = [](client* c1) {

    int recvbuflen = DEFAULT_BUFLEN;
    char recvbuf[DEFAULT_BUFLEN];
    int iResult = 1;

    while (server_running && iResult >0 ) {
        int iResult = recv(c1->ClientSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0 ) {

            ostringstream ss;
            ss << "SOCKET #" << c1->ClientSocket << ": " << recvbuf << "\r";
            string strOut = ss.str();

            cout << "client with socket id : " << c1->ClientSocket << " send a message ." << endl;
            sem1.wait();
            message_q.push(queue_type(strOut, c1));
            sem1.signal();
        }
        else {
            cout << "client with socket id : " <<c1->ClientSocket<<" left ." << endl;
            clients.erase(c1);
            return;
        }
    }
};


class server {
public:
    SOCKET ListenSocket;
    
    map<int, std::thread> vecofthreads;
    int id;

    server() {
        ListenSocket = INVALID_SOCKET;
        id = 0;
        vecofthreads.clear();
    }
    bool on_startup() {
        WSADATA wsaData;
        int iResult;

        struct addrinfo* result = NULL;
        struct addrinfo hints;

        int iSendResult;

        // Initialize Winsock
        iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (iResult != 0) {
            printf("WSAStartup failed with error: %d\n", iResult);
            return 1;
        }

        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        hints.ai_flags = AI_PASSIVE;

        // Resolve the server address and port
        iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
        if (iResult != 0) {
            printf("getaddrinfo failed with error: %d\n", iResult);
            WSACleanup();
            return 1;
        }

        // Create a SOCKET for connecting to server
        ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (ListenSocket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            freeaddrinfo(result);
            WSACleanup();
            return 1;
        }

        // Setup the TCP listening socket
        iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            printf("bind failed with error: %d\n", WSAGetLastError());
            freeaddrinfo(result);
            closesocket(ListenSocket);
            WSACleanup();
            return 1;
        }

        freeaddrinfo(result);

        iResult = listen(ListenSocket, SOMAXCONN);
        if (iResult == SOCKET_ERROR) {
            printf("listen failed with error: %d\n", WSAGetLastError());
            closesocket(ListenSocket);
            WSACleanup();
            return 1;
        }
        return 0;
    }

    // it runs when a new client comes 
    void on_newClient(SOCKET clientsocket) {
        cout << "client with socket id : " << clientsocket << " trying to enter into server . " << endl;

        // Send a welcome message to the connected client
        string message = "Welcome to the Awesome Chat Server!\r";
        int iResult = send(clientsocket, message.c_str(), message.size() + 1, 0);

        if (iResult > 0) {
            
            client* c1 = new client(clientsocket);
            ostringstream ss;
            ss << "SOCKET #" << clientsocket << ": joined the chat" << "\r";
            string strOut = ss.str();
            message_q.push(queue_type(strOut, c1));
            cout << "SOCKET #" << clientsocket << ": connected to  server \n";

            clients[c1] = id;
            vecofthreads[id] = std::thread(func2, c1);
            ++id;
        }
        else {
            closesocket(clientsocket);
            WSACleanup();
        }
    }

    ~server() {
        server_running = false;
        
        closesocket(ListenSocket);
        for (auto it : clients) {

            int iResult = shutdown(it.first->ClientSocket, SD_SEND);
            // cleanup
            closesocket(it.first->ClientSocket);
            WSACleanup();
            delete (it.first);

            vecofthreads[it.second].join();
            vecofthreads.erase(it.second);
            clients.erase(it.first);
            
        }
        clients.clear();
        vecofthreads.clear();
    }
};


void fun1(server* s1) {
    while (server_running) {

        // Accept a client socket
        SOCKET clientsocket = accept(s1->ListenSocket, NULL, NULL);
        if (clientsocket != INVALID_SOCKET) {
            s1->on_newClient(clientsocket);
        }
    }
}

void fun3(server* s1) {
    while (server_running) {

        int len = message_q.size();

        if (len > 0) {

            auto front = message_q.front();
            message_q.pop();

            for (auto it : clients) {
                if (front.newclient != it.first) {
                    int iResult = send(it.first->ClientSocket, front.message.c_str(), front.message.length() + 1, 0);
                    if (iResult == SOCKET_ERROR) {
                        cout << "send failed with error: " << WSAGetLastError() << endl;

                        clients.erase(it.first);
                        delete it.first ;
                    }
                }
            }
            cout << "message has been sent to all clients " << endl;
        }
    }
}

int __cdecl main(void)
{
    server* s1 = new server();
    if (s1->on_startup() == 0) {
        cout << "Server Started Successfully " << endl;
    }
    else {
        cout << "Error in starting server . \nRetry starting again !!" << endl;
        return 0;
    }

    // thread th1 for accepting connection to clients 
    std::thread th1(fun1, s1);
    // thread2 for sending messages from message queue to clients 
    std::thread th2(fun3, s1);

    if (th1.joinable()) th1.join();
    if (th2.joinable()) th2.join();

    delete s1;
    return 0;
}