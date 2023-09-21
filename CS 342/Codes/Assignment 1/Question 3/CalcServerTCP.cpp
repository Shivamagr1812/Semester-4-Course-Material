#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <bits/stdc++.h>

using namespace std;

// Function to perform mathematical calculations on the expression
double performCalculation(const string &expression) {
    int length = expression.length();
    if (length == 0) return 0;

    double currentNumber = 0, lastNumber = 0, result = 0;
    char sign = '+';

    for (int i = 0; i < length; i++) {
        string temp;
        bool isNegative = false;

        if (expression[i] == '-') {
            isNegative = true;
            i++;
        }

        while (i < length && (isdigit(expression[i]) || expression[i] == '.')) {
            temp.push_back(expression[i]);
            i++;
        }

        currentNumber = stod(temp);
        if (isNegative) {
            currentNumber = -currentNumber;
        }

        if (!isdigit(expression[i]) && !isspace(expression[i]) || i == length - 1) {
            if (sign == '+' || sign == '-') {
                result += lastNumber;
                lastNumber = (sign == '+') ? currentNumber : -currentNumber;
            } else if (sign == '*') {
                lastNumber = lastNumber * currentNumber;
            } else if (sign == '/') {
                lastNumber = lastNumber / currentNumber;
            } else if (sign == '^') {
                lastNumber = pow(lastNumber, currentNumber);
            }

            sign = expression[i];
            currentNumber = 0;
        }
    }

    result += lastNumber;
    return result;
}

// Function to set up a server socket and handle client connections
void startServer(int port) {
    int serverSock, acceptSock;
    serverSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    if (serverSock < 0) {
        perror("Failed to create socket");
        return;
    } else {
        cout << "Socket creation successful" << endl;
    }

    sockaddr_in service;
    int servlen = sizeof(service);
    service.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &service.sin_addr.s_addr);
    service.sin_port = htons(port);

    if (bind(serverSock, (struct sockaddr *) &service, sizeof(service)) < 0) {
        perror("bind() failed");
        return;
    } else {
        cout << "bind() successful" << endl;
    }

    if (listen(serverSock, 4) < 0) {
        perror("listen() failed");
        return;
    } else {
        cout << "listen() successful" << endl;
    }

    while (true) {
        acceptSock = accept(serverSock, (struct sockaddr *) &service, (socklen_t *) &servlen);
        if (acceptSock < 0) {
            perror("accept failed");
            return;
        }

        while (true) {
            char buffer[200];
            if (recv(acceptSock, buffer, 200, 0) < 0) {
                perror("Receive unsuccessful");
                return;
            }

            string temp(buffer);
            if (temp == "-1") {
                close(acceptSock);
                return;
            }

            double answer = performCalculation(temp);
            string resultStr = to_string(answer);
            strcpy(buffer, resultStr.c_str());

            if (send(acceptSock, buffer, 200, 0) < 0) {
                perror("Send unsuccessful");
                return;
            }
        }
    }
    close(serverSock);
}

int main(int argc, char *args[]) {
    if (argc < 2) {
        cout << "Usage: " << args[0] << " <port>" << endl;
        return 1;
    }

    int port = stoi(args[1]);
    startServer(port);

    return 0;
}
