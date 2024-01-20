#define WIN32_LEAN_AND_MEAN
#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

using namespace std;

atomic<bool> serverRunning(true);

// Function to send messages to the server
void sender(SOCKET connectSocket) {
    string sendBuffer;
    while (serverRunning) {
        getline(cin, sendBuffer);
        sendBuffer += "\r";
        int sendResult = send(connectSocket, sendBuffer.c_str(), sendBuffer.length()+1, 0);
        if (sendResult == SOCKET_ERROR) {
            cerr << "send failed with error: " << WSAGetLastError() << "\n";
            closesocket(connectSocket);
            WSACleanup();
            serverRunning = false;
            break;
        }
    }
}

// Function to receive messages from the server
void receiver(SOCKET connectSocket) {
    int recvBufferSize = DEFAULT_BUFLEN;
    char recvBuffer[DEFAULT_BUFLEN];

    while (serverRunning) {
        int recvResult = recv(connectSocket, recvBuffer, recvBufferSize, 0);
        if (recvResult > 0) {
            string message = recvBuffer ;
            cout << message << "\n";
        }
        else {
            if (recvResult == 0) {
                cout << "Connection closed\n";
            }
            else {
                cerr << "recv failed with error: " << WSAGetLastError() << "\n";
            }
            serverRunning = false;
            return;
        }
    }
}

// Client class to encapsulate client-related functionality
class Client {
public:
    SOCKET connectSocket;
    string name;

    Client(const string& serverIP) : connectSocket(INVALID_SOCKET) {
        cout << "Enter your name: ";
        cin >> name;
        if (!onStartup(serverIP)) {
            cerr << "Error in starting up the client.\n";
            exit(1);
        }
    }

    // Initialize Winsock, create socket, and connect to the server
    bool onStartup(const string& serverIP) {
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            cerr << "WSAStartup failed with error: " << result << "\n";
            return false;
        }

        struct addrinfo* resultAddr = NULL;
        struct addrinfo hints;

        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        result = getaddrinfo(serverIP.c_str(), DEFAULT_PORT, &hints, &resultAddr);
        if (result != 0) {
            cerr << "getaddrinfo failed with error: " << result << "\n";
            WSACleanup();
            return false;
        }

        for (struct addrinfo* ptr = resultAddr; ptr != NULL; ptr = ptr->ai_next) {
            connectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
            if (connectSocket == INVALID_SOCKET) {
                cerr << "socket failed with error: " << WSAGetLastError() << "\n";
                WSACleanup();
                return false;
            }

            result = connect(connectSocket, ptr->ai_addr, static_cast<int>(ptr->ai_addrlen));
            if (result == SOCKET_ERROR) {
                closesocket(connectSocket);
                connectSocket = INVALID_SOCKET;
                continue;
            }
            break;
        }

        freeaddrinfo(resultAddr);

        if (connectSocket == INVALID_SOCKET) {
            cerr << "Unable to connect to the server.\n";
            WSACleanup();
            return false;
        }
        else {
            cout << "Connected to the server.\n";
        }

        return true;
    }

    // Destructor to close the socket and clean up
    ~Client() {
        serverRunning = false;
        shutdown(connectSocket, SD_SEND);
        closesocket(connectSocket);
        WSACleanup();
    }
};

int main(int argc, char** argv) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <server IP>\n";
        return 1;
    }

    Client* client = new Client(argv[1]);

    // Start sender and receiver threads
    thread senderThread(sender, client->connectSocket);
    thread receiverThread(receiver, client->connectSocket);

    // Wait for threads to finish
    if (senderThread.joinable()) senderThread.join();
    if (receiverThread.joinable()) receiverThread.join();

    delete client;
    return 0;
}