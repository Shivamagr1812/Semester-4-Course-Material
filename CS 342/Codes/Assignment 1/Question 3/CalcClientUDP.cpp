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
char *HostnameToIp(const string &hostname) {
    hostent *hostInfo = gethostbyname(hostname.c_str());
    if (hostInfo)
        return (char *)(inet_ntoa(*(in_addr *)(hostInfo->h_addr_list[0])));
    return nullptr;
}

// Function to perform the client operations
void RunClient(const char *ip, int port) {
    int clientSock;
    clientSock = socket(AF_INET, SOCK_DGRAM, 0);
    
    if (clientSock < 0)
        cout << "Failed to create socket" << endl;
    else
        cout << "Socket creation successful" << endl;

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port); 
    
    if (inet_pton(AF_INET, ip, &serverAddr.sin_addr.s_addr) < 0)
        cout << "Error in converting IP address" << endl;
    
    socklen_t len;

    while (true) {
        cout << endl;
        char buffer[200];
        cout << "Enter Expression: ";
        
        cin.getline(buffer, sizeof(buffer));
        if (sendto(clientSock, buffer, sizeof(buffer), MSG_CONFIRM,
                    (const struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
            cout << "Send Unsuccessful" << endl;
            return;
        }

        string temp(buffer);
        if (temp == "-1")
            return;

        if (recvfrom(clientSock, buffer, sizeof(buffer), MSG_WAITALL,
                     (struct sockaddr *) &serverAddr, &len) < 0) {
            cout << "Unsuccessful" << endl;
            return;
        }

        printf("%s", buffer);
        cout << endl;
    }
}

int main(int argc, char *args[]) {
    if (argc < 3) {
        cout << "Usage: " << args[0] << " <hostname> <port>" << endl;
        return 1;
    }

    string hostname = args[1];
    int port = stoi(args[2]);
    char *ip = HostnameToIp(hostname);

    if (ip == nullptr) {
        cout << "Failed to resolve hostname" << endl;
        return 1;
    }

    RunClient(ip, port);

    return 0;
}
