#define WIN32_LEAN_AND_MEAN
#include<stdio.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include<iostream>
#include<string>
#include<thread>

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")


#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

using namespace std;


void fun1(bool* server_running , SOCKET ConnectSocket) {
    string sendbuf;
    while (*server_running) {
        getline(cin, sendbuf);
        sendbuf = sendbuf + "\r";
        int iResult = send(ConnectSocket , sendbuf.c_str(), sendbuf.length() + 1, 0);
        if (iResult == SOCKET_ERROR) {
            printf("send failed with error: %d\n", WSAGetLastError());
            closesocket(ConnectSocket);
            WSACleanup();
            *server_running = false;
            break;
        }
    }
}

void fun2(bool* server_running , SOCKET ConnectSocket) {
    int recvbuflen = DEFAULT_BUFLEN;
    char recvbuf[DEFAULT_BUFLEN];

    int iResult = 1;
    // Receive until the peer closes the connection
    while (iResult > 0 && *server_running ) {

        iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0) {
            string message = (string)recvbuf;
            cout << message << endl;
        }
        else {
            if (iResult == 0) {
                cout << "Connection closed\n";
            }
            else {
                cout << "recv failed with error : " << WSAGetLastError() << endl;
            }
            *server_running = false;
            return;
        }
    }
}

class client{
    public:
    bool server_running ;
    SOCKET ConnectSocket ;
    string name ;
    client(){
        ConnectSocket  = INVALID_SOCKET;
        server_running = true;
        cout<<"Enter your name : ";
        cin>>name;
    }
    bool on_startup() {
        WSADATA wsaData;
        struct addrinfo* result = NULL,
            * ptr = NULL,
            hints;

        int iResult;

        // Initialize Winsock
        iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (iResult != 0) {
            printf("WSAStartup failed with error: %d\n", iResult);
            return 1;
        }
        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        // Resolve the server address and port
        iResult = getaddrinfo("127.0.0.1", DEFAULT_PORT, &hints, &result);
        if (iResult != 0) {
            printf("getaddrinfo failed with error: %d\n", iResult);
            WSACleanup();
            return 1;
        }

        // Attempt to connect to an address until one succeeds
        for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

            // Create a SOCKET for connecting to server
            ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

            if (ConnectSocket == INVALID_SOCKET) {
                printf("socket failed with error: %ld\n", WSAGetLastError());
                WSACleanup();
                return 1;
            }

            // Connect to server.
            iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
            if (iResult == SOCKET_ERROR) {
                closesocket(ConnectSocket);
                ConnectSocket = INVALID_SOCKET;
                continue;
            }
            break;
        }

        freeaddrinfo(result);

        if (ConnectSocket == INVALID_SOCKET) {
            printf("Unable to connect to server!\n");
            WSACleanup();
            return 1;
        }
        else {
            cout << "Connected to server " << endl;
        }
        return 0;
    }
    ~client(){
        
        server_running = false;
        // shutdown the send half of the connection since no more data will be sent
        int iResult = shutdown(ConnectSocket , SD_SEND);
        // cleanup
        closesocket(ConnectSocket );
        WSACleanup();
        
    }
};

int __cdecl main(int argc, char** argv)
{   

    // creating a client 
    client * c1 = new client();
    if (c1->on_startup()) return 0;
   

    std::thread th1(fun1 , &(c1->server_running) , c1->ConnectSocket );
    std::thread th2(fun2 , &(c1->server_running) , c1->ConnectSocket );

    if (th1.joinable() || th2.joinable()) {
        th1.join();
        th2.join();
        cout << "server Closed " << endl;
    }

    delete c1;

    return 0;
}

