#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cerrno>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include "base64.cpp"

using namespace std;

int CreateServerSocket(int portNumber) {
    int serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket < 0) {
        cout << "Failed to create socket." << endl;
        exit(1);
    }
    cout << "Socket creation successful." << endl;

    sockaddr_in serverAddress;
    int serverAddressLength = sizeof(serverAddress);

    serverAddress.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr.s_addr);
    serverAddress.sin_port = htons(portNumber);

    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        cout << "Binding failed." << endl;
        exit(1);
    }
    cout << "Binding successful." << endl;

    if (listen(serverSocket, 5) < 0) {
        cout << "Listening failed." << endl;
        exit(1);
    }
    cout << "Listening for incoming connections." << endl;

    return serverSocket;
}

void HandleNewConnection(int newClientSocket, vector<int> &connectedClients) {
    cout << "New client connected: " << newClientSocket << endl;
    connectedClients.push_back(newClientSocket);
}

void HandleClientData(int clientSocketDescriptor, vector<int> &connectedClients) {
    char receivedBuffer[256];
    if (recv(clientSocketDescriptor, receivedBuffer, 200, 0) < 0) {
        cout << "Failed to receive data from client." << endl;
        return;
    }

    string receivedTemp(receivedBuffer);
    string decodedData = b64decode(receivedBuffer, receivedTemp.length());

    if (decodedData[0] == '3') {
        auto it = find(connectedClients.begin(), connectedClients.end(), clientSocketDescriptor);
        if (it != connectedClients.end()) {
            cout << "Client " << *it << " disconnected." << endl;
            connectedClients.erase(it);
        }
        return;
    }

    for (int i = 2; i < decodedData.length(); i++)
        cout << decodedData[i];
    cout << endl;

    string response = "2 Acknowledgement";
    strcpy(receivedBuffer, response.c_str());
    string encodedResponse = b64encode(receivedBuffer, response.length());
    strcpy(receivedBuffer, encodedResponse.c_str());

    if (send(clientSocketDescriptor, receivedBuffer, 200, 0) < 0) {
        cout << "Failed to send response." << endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "Usage: " << argv[0] << " <port>" << endl;
        return 1;
    }

    int portNumber = stoi(argv[1]);

    int serverSocket = CreateServerSocket(portNumber);
    vector<int> connectedClients;
    fd_set masterSet;

    while (true) {
        FD_ZERO(&masterSet);
        FD_SET(serverSocket, &masterSet);
        int maxSocketDescriptor = serverSocket;

        for (auto clientSocketDescriptor : connectedClients) {
            FD_SET(clientSocketDescriptor, &masterSet);
            maxSocketDescriptor = max(maxSocketDescriptor, clientSocketDescriptor);
        }

        int activity = select(maxSocketDescriptor + 1, &masterSet, NULL, NULL, NULL);
        if (activity < 0 && (errno != EINTR)) {
            cout << "Error during select." << endl;
            return 1;
        }

        if (FD_ISSET(serverSocket, &masterSet)) {
            int newClientSocket;
            if ((newClientSocket = accept(serverSocket, NULL, NULL)) < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }
            HandleNewConnection(newClientSocket, connectedClients);
        }

        for (int i = 0; i < connectedClients.size(); i++) {
            int clientSocketDescriptor = connectedClients[i];
            if (FD_ISSET(clientSocketDescriptor, &masterSet)) {
                HandleClientData(clientSocketDescriptor, connectedClients);
            }
        }
    }

    return 0;
}
