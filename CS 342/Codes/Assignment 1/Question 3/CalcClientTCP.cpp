#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <iostream>
#include <string>

using namespace std;

// Function to convert hostname to IP address
char *HostToIp(const string &host) {
    hostent *hostname = gethostbyname(host.c_str());
    if (hostname)
        return (char *)(inet_ntoa(*(in_addr *)(hostname->h_addr_list[0])));
    return nullptr;
}

// Function to establish connection with the server
int ConnectToServer(const char *ip, int port) {
    int clientSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSock < 0) {
        perror("Failed to create socket");
        return -1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &serverAddr.sin_addr);

    if (connect(clientSock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Connection failed");
        return -1;
    }

    return clientSock;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        cout << "Usage: " << argv[0] << " <hostname> <port>" << endl;
        return 1;
    }

    string hostname = argv[1];
    int port = stoi(argv[2]);
    char *ip = HostToIp(hostname);

    if (ip == nullptr) {
        cout << "Failed to resolve hostname" << endl;
        return 1;
    }

    int clientSock = ConnectToServer(ip, port);
    if (clientSock == -1) {
        return 1;
    }

    while (true) {
        char buffer[200];
        cout << "Enter Expression: ";
        cin.getline(buffer, sizeof(buffer));

        if (strcmp(buffer, "-1") == 0) {
            break;
        }

        if (send(clientSock, buffer, sizeof(buffer), 0) < 0) {
            perror("Send Unsuccessful");
            break;
        }

        if (recv(clientSock, buffer, sizeof(buffer), 0) < 0) {
            perror("Receive Unsuccessful");
            break;
        }

        cout << "Answer: " << buffer << endl;
    }

    close(clientSock);
    return 0;
}
