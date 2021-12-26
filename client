#include <arpa/inet.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
using namespace std;

bool transfer(string name, int portNum, char payment[], string data, const char* payeeIP, int payeePort);
void receiving(int p2p_fd);
void* receive_thread(void* p2p_fd);

int client_fd = 0;

int main(int argc ,char *argv[])
{
    int p2p_fd;
    struct sockaddr_in server, address;
    
    while(true)
    {
        // Create socket
        client_fd = socket(AF_INET, SOCK_STREAM, 0);
        if(client_fd == -1)
        {
            printf("Could not create socket");
        }
        p2p_fd = socket(AF_INET, SOCK_STREAM, 0);
        if(p2p_fd == -1)
        {
            printf("Could not create socket");
        }
        
        // Server info
        char serverIP[80] = {0};
        cout << "Please enter the server's IP address:";
        scanf("%s", serverIP);
        int serverPortNum = 0;
        cout << "Please enter the server's port number:";
        scanf("%i", &serverPortNum);
        server.sin_addr.s_addr = inet_addr(serverIP);
        server.sin_family = AF_INET;
        server.sin_port = htons(serverPortNum);

        // Connect to remote server
        if(connect(client_fd, (struct sockaddr *)&server, sizeof(server)) < 0)
        {
            puts("connect error");
            return 1;
        }
        char server_reply[20000] = {0};
        recv(client_fd, server_reply, 20000, 0);
        puts(server_reply);
        string temp = server_reply;
        if(temp.find("Connection success") != string::npos)
        {
            break;
        }
        close(client_fd);
    }
    
    // Get input from user
    string loggedIn, list;
    int portNum = 0;
    while(true)
    {
        cout << "Please enter the function you'd like to use(register, login, list, transfer, exit):";
        string function;
        cin >> function;
        
        string name, payee, data;
        int payment = 0;
        bool transferSuccess = true;
        if(function == "register")
        {
            cout << "Please enter your account name:";
            cin >> name;
            data = "REGISTER#" + name;
        }
        else if(function == "login")
        {
            if(!loggedIn.empty())
            {
                cout << "Already logged in!\n";
                continue;
            }
            cout << "Please enter your account name:";
            cin >> name;
            cout << "Please enter your port number:";
            cin >> portNum;
            char buffer[10];
            sprintf(buffer, "%d", portNum);
            data = name + "#" + buffer;
        }
        else if(function == "list")
        {
            data = "List";
        }
        else if(function == "transfer")
        {
            if(loggedIn.empty())
            {
                cout << "Please log in first\n";
                continue;
            }
            cout << "Please enter your account name:";
            cin >> name;
            if(name != loggedIn)
            {
                cout << "Wrong account name!\n";
                continue;
            }
            cout << "Please enter the transfer amount:";
            cin >> payment;
            if(payment <= 0)
            {
                cout << "Invalid transfer amount!\n";
                continue;
            }
            cout << "Please enter the payee account name:";
            cin >> payee;
            if(name == payee)
            {
                cout << "Cannot transfer to yourself!\n";
                continue;
            }
            size_t IPStart = list.find(payee) + payee.size() + 1;
            size_t IPEnd = list.find('#', IPStart);
            if(IPStart == string::npos || IPEnd == string::npos)
            {
                cout << "Please use the list function first and try again\n";
                continue;
            }
            string str;
            str.assign(list, IPStart, IPEnd - IPStart);
            const char* payeeIP = {0};
            payeeIP = str.c_str();
            size_t portStart = IPEnd + 1;
            size_t portEnd = list.find('\n', portStart);
            string str2;
            str2.assign(list, portStart, portEnd - portStart);
            int payeePort = atoi(str2.c_str());
            char buffer[10] = {0};
            sprintf(buffer, "%d", payment);
            data = name + "#" + buffer + "#" + payee;
            transferSuccess = transfer(name, portNum, buffer, data, payeeIP, payeePort);
            if(transferSuccess)
            {
                cout << "Waiting for the server to respond...\n";
                //Receive a reply from the server
                char server_reply[20000] = {0};
                recv(client_fd, server_reply, 20000, 0);
                puts(server_reply);
                cout << "Please check your account.\n";
                continue;
            }
        }
        else if(function == "exit")
        {
            data = "Exit";
        }
        else
        {
            cout << "Input format error\n";
            continue;
        }
        
        //Send some data
        const char* message = data.c_str();
        char server_reply[20000] = {0};
        if(function != "transfer")
        {
            if(send(client_fd, message, strlen(message), 0) < 0)
            {
                puts("Send failed");
            }
            cout << "Waiting for the server to respond...\n";
            //Receive a reply from the server
            if(recv(client_fd, server_reply, 20000, 0) < 0)
            {
                puts("recv failed");
            }
            puts(server_reply);
        }
        
        if(function == "login")
        {
            if(strstr(server_reply, "220 AUTH_FAIL") == NULL)
            {
                close(p2p_fd);
                loggedIn = name;
                p2p_fd = socket(AF_INET, SOCK_STREAM, 0);
                if(p2p_fd == -1)
                {
                    printf("Could not create socket");
                }
                // Bind socket and listen
                address.sin_family = AF_INET;
                address.sin_addr.s_addr = INADDR_ANY;
                address.sin_port = htons(portNum);
                if(bind(p2p_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
                {
                    perror("bind failed");
                    exit(EXIT_FAILURE);
                }
                if(listen(p2p_fd, 5) < 0)
                {
                    perror("listen");
                    exit(EXIT_FAILURE);
                }
                int ch;
                pthread_t tid;
                pthread_create(&tid, NULL, &receive_thread, &p2p_fd); //Creating thread to keep receiving message in real time
            }
        }
        else if(function == "list")
        {
            list = server_reply;
        }
        else if(function == "exit")
        {
            close(client_fd);
            return 0;
        }
    }
    
    return 0;
}

//Sending messages to port
bool transfer(string name, int portNum, char payment[], string data, const char* payeeIP, int payeePort)
{
    char buffer[2000] = {0};
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char hello[1024] = {0};
    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return false;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(payeeIP);
    serv_addr.sin_port = htons(payeePort);

    if(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConnection Failed. Please check list again.\n");
        return false;
    }

    sprintf(buffer, "%s is trying to transfer $%s to you\n%s", name.c_str(), payment, data.c_str());
    send(sock, buffer, sizeof(buffer), 0);
    printf("\nMessage sent\n");
    close(sock);
    return true;
}

//Calling receiving every 2 seconds
void* receive_thread(void* p2p_fd)
{
    int s_fd = *((int*)p2p_fd);
    while(true)
    {
        sleep(1);
        receiving(s_fd);
    }
}

//Receiving messages on our port
void receiving(int p2p_fd)
{
    struct sockaddr_in address;
    int valread;
    char buffer[2000] = {0};
    int addrlen = sizeof(address);
    fd_set current_sockets, ready_sockets;

    //Initialize my current set
    FD_ZERO(&current_sockets);
    FD_SET(p2p_fd, &current_sockets);
    int k = 0;
    while(true)
    {
        k++;
        ready_sockets = current_sockets;

        if(select(FD_SETSIZE, &ready_sockets, NULL, NULL, NULL) < 0)
        {
            perror("Error");
            exit(EXIT_FAILURE);
        }

        for(int i = 0; i < FD_SETSIZE; i++)
        {
            if(FD_ISSET(i, &ready_sockets))
            {
                if(i == p2p_fd)
                {
                    int client_socket;

                    if((client_socket = accept(p2p_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
                    {
                        perror("accept");
                        exit(EXIT_FAILURE);
                    }
                    FD_SET(client_socket, &current_sockets);
                }
                else
                {
                    valread = recv(i, buffer, sizeof(buffer), 0);
                    if(buffer[0] != static_cast<char>(0))
                    {
                        string transferData;
                        int i = 0;
                        while(true)
                        {
                            if(buffer[i] == '\n')
                            {
                                transferData = buffer + i + 1;
                                break;
                            }
                            i++;
                        }
                        string str;
                        str.assign(buffer, i);
                        printf("\n%s\n", str.c_str());
                        
                        const char* message = transferData.c_str();
                        if(send(client_fd, message, strlen(message), 0) < 0)
                        {
                            puts("Send failed");
                        }
                    }
                    cout << "Please check your account.\n";
                    FD_CLR(i, &current_sockets);
                }
            }
        }
        
        if (k == (FD_SETSIZE * 2))
        {
            break;
        }
    }
}
