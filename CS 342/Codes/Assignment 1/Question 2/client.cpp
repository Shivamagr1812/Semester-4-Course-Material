#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <iostream>

using namespace std;

// Function to establish a connection with the server
int ConnectToServer(const char* ipAddress, int port) {
    int clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket < 0) {
        cout << "Failed to create socket." << endl;
        exit(EXIT_FAILURE);
    }

    sockaddr_in service;
    service.sin_family = AF_INET;
    inet_pton(AF_INET, ipAddress, &service.sin_addr.s_addr);
    service.sin_port = htons(port);

    if (connect(clientSocket, (struct sockaddr*)&service, sizeof(service)) < 0) {
        cout << "Connection failed." << endl;
        exit(EXIT_FAILURE);
    }

    return clientSocket;
}

// Function to receive and display the welcome message
void ReceiveWelcomeMessage(int clientSocket) {
    char buffer[256];
    if (recv(clientSocket, buffer, 256, 0) < 0) {
        cout << "Unsuccessful" << endl;
        close(clientSocket);
        exit(EXIT_FAILURE);
    }
    cout << buffer << endl;
}

// Function to interact with the server
void InteractWithServer(int clientSocket) {
    char buffer[256];
    while (true) {
        cout << " " << endl;
        cout << "Enter Message: ";
        cin.getline(buffer, 256);
        string temp(buffer);

        // Send the user's message to the server
        if (send(clientSocket, buffer, 256, 0) < 0) {
            cout << "Send Unsuccessful" << endl;
            close(clientSocket);
            exit(EXIT_FAILURE);
        }

        // Check if the user wants to exit
        if (temp == "exit") {
            close(clientSocket);
            return;
        }

        // Receive and display the server's response
        if (recv(clientSocket, buffer, 256, 0) < 0) {
            cout << "Unsuccessful" << endl;
            close(clientSocket);
            exit(EXIT_FAILURE);
        }
        cout << buffer << endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cout << "Usage: " << argv[0] << " <server_ip> <port>" << endl;
        return 0;
    }

    const char* ipAddress = argv[1];
    int port = stoi(argv[2]);

    int clientSocket = ConnectToServer(ipAddress, port);

    ReceiveWelcomeMessage(clientSocket);

    InteractWithServer(clientSocket);

    return 0;
}
