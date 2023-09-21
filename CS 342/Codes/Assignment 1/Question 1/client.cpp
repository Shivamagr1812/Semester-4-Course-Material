#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "base64.cpp"
#include <iostream>
#include <string>

using namespace std;

int CreateClientSocket(const char* serverIP, int port) {
    int clientSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSock < 0) {
        cout << "Failed to create socket." << endl;
        exit(1);
    }
    cout << "Socket creation successful." << endl;

    sockaddr_in serverAddress;
    int serverAddressLength = sizeof(serverAddress);

    serverAddress.sin_family = AF_INET;
    inet_pton(AF_INET, serverIP, &serverAddress.sin_addr.s_addr);
    serverAddress.sin_port = htons(port);

    if (connect(clientSock, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        cout << "Connection failed." << endl;
        exit(1);
    }
    cout << "Connected to server." << endl;

    return clientSock;
}

void SendAndReceiveMessages(int clientSocket) {
    while (true) {
        cout << endl;
        char buffer[200];
        cout << "Enter Message: ";

        cin.getline(buffer, 200);
        string temp(buffer);
        string encodedMessage = b64encode(buffer, temp.length());
        strcpy(buffer, encodedMessage.c_str());

        if (send(clientSocket, buffer, 200, 0) < 0) {
            cout << "Failed to send message." << endl;
            exit(1);
        }

        if (temp[0] == '3') {
            cout << "Connection terminated." << endl;
            return;
        }

        if (recv(clientSocket, buffer, 200, 0) < 0) {
            cout << "Failed to receive response." << endl;
            exit(1);
        }

        string temp2(buffer);
        string decodedResponse = b64decode(buffer, temp2.length());

        cout << "Server's response: ";
        for (int i = 2; i < decodedResponse.length(); i++)
            cout << decodedResponse[i];
        cout << endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cout << "Usage: " << argv[0] << " <server_ip> <port>" << endl;
        return 1;
    }

    const char* serverIP = argv[1];
    int portNumber = stoi(argv[2]);

    int clientSocket = CreateClientSocket(serverIP, portNumber);
    SendAndReceiveMessages(clientSocket);

    return 0;
}
