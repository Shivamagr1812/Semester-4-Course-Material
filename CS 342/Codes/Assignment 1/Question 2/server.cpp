#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <thread>
#include <iostream>
#include <string>

using namespace std;

// Function to handle communication with each client
void HandleClient(int clientSocket) {
    while (true) {
        char buffer[256];
        // Receive message from the client
        recv(clientSocket, buffer, 256, 0);
        string receivedMessage(buffer);
        
        if (receivedMessage == "exit") {
            // Close the client socket and exit the thread
            close(clientSocket);
            return;
        }

        // Display the received message
        cout << "Client " << clientSocket << " Says: " << receivedMessage << endl;
        
        // Reply to the client
        cout << "Enter Message to reply to Client " << clientSocket << ": ";
        cin.getline(buffer, 256);
        send(clientSocket, buffer, 256, 0);
    }
}

int CreateServerSocket(int port) {
    int serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket < 0) {
        cout << "Failed to create socket." << endl;
        exit(1);
    }
    cout << "Socket creation successful." << endl;

    sockaddr_in service;
    int serviceLength = sizeof(service);

    // Set up the server address structure
    service.sin_family = AF_INET;
    service.sin_addr.s_addr = INADDR_ANY;
    service.sin_port = htons(port);

    // Bind the socket to the specified address and port
    if (bind(serverSocket, (struct sockaddr*)&service, sizeof(service)) < 0) {
        cout << "bind() failed." << endl;
        exit(1);
    }
    cout << "bind() is successful." << endl;

    // Listen for incoming connections
    if (listen(serverSocket, 5) < 0) {
        cout << "listen() failed." << endl;
        exit(1);
    }
    cout << "listen() is successful." << endl;

    return serverSocket;
}

int AcceptClientConnection(int serverSocket) {
    sockaddr_in clientAddr;
    socklen_t clientAddrLength = sizeof(clientAddr);
    int newClientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLength);
    
    if (newClientSocket < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    return newClientSocket;
}

void RunServer(int serverSocket) {
    while (true) {
        // Accept a new client connection
        int newClientSocket = AcceptClientConnection(serverSocket);

        char* welcome = "Hello, welcome to the server";
        if (send(newClientSocket, welcome, strlen(welcome), 0) != strlen(welcome)) {
            cout << "Send Error" << endl;
        }

        cout << "Another Client Added on: " << newClientSocket << endl;

        // Create a new thread to handle the client
        thread clientThread(HandleClient, newClientSocket);
        clientThread.detach();
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "Usage: " << argv[0] << " <port>" << endl;
        return 0;
    }

    int port = stoi(argv[1]);

    int serverSocket = CreateServerSocket(port);
    RunServer(serverSocket);

    // Close the server socket
    close(serverSocket);

    return 0;
}
