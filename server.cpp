#include <arpa/inet.h> // inet_addr
#include <cstring>
#include <iostream>
#include <pthread.h> // for threading, link with lpthread
#include <stdio.h>
#include <stdlib.h> // strlen
#include <string.h> // strlen
#include <sys/socket.h>
#include <unistd.h> // write
#include <vector>
using namespace std;

class clientInfo
{
public:
    string name;
    bool loggedIn;
    string IPAddress;
    int portNumber;
    int accountBalance;
    
    clientInfo(): name(), loggedIn(false), IPAddress(), portNumber(0), accountBalance(10000){}
    clientInfo(string name): name(name), loggedIn(false), IPAddress(), portNumber(0), accountBalance(10000){}
};

class clientSockInfo
{
public:
    bool used;
    int* new_sock;
    string name;
    
    clientSockInfo(bool used): used(used), new_sock(), name(){}
};

char* clientIP = new char[20];
int clientCount = 0;
int currentSocketIndex = 0;
vector<clientInfo> clientList;
vector<clientSockInfo> clientSockList;

// the thread function
void* connection_handler(void*);
void printList(string& data, int clientIndex);

int main(int argc, char* argv[])
{
    int socket_desc, client_sock, c;
    struct sockaddr_in server, client;
    
    // Create socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_desc == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");

    // Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    cout << "Please enter the port number you would like to use:";
    int serverPortNum = 0;
    cin >> serverPortNum;
    server.sin_port = htons(serverPortNum);
    
    // Bind
    if(bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        // print the error message
        perror("Bind failed. Error");
        return 1;
    }
    puts("Bind done");
    
    // Listen
    listen(socket_desc, 3);
        
    // Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
    for(int i = 0; i < 3; i++)
    {
        clientSockInfo cSock(false);
        clientSockList.push_back(cSock);
    }
    while((client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)))
    {
        if(clientCount >= 3)
        {
            cout << "Too many people online.\n";
            char message[100] = "Too many people online. Please try again later\n";
            write(client_sock, message, strlen(message));
            close(client_sock);
            continue;
        }
        else
        {
            char message[100] = "Connection success!\n";
            write(client_sock, message, strlen(message));
        }
        puts("Connection accepted!");
        
        pthread_t sniffer_thread;
        int index = 0;
        for(; index < 3; index++)
        {
            if(clientSockList[index].used == false)
            {
                clientSockList[index].new_sock = static_cast<int*>(malloc(1));
                *(clientSockList[index].new_sock) = client_sock;
                clientSockList[index].used = true;
                currentSocketIndex = index;
                break;
            }
        }
        
        for(int i = 0; i < 20; i++)
        {
            clientIP[i] = 0;
        }
        clientIP = inet_ntoa(client.sin_addr);
        
        if(pthread_create(&sniffer_thread, NULL, connection_handler, (void*)clientSockList[index].new_sock) < 0)
        {
            perror("Could not create thread");
            return 1;
        }
        
        // Now join the thread , so that we don't terminate before the thread
        // pthread_join(sniffer_thread, NULL);
        puts("Handler assigned");
        
        clientCount++;
    }
    
    if(client_sock < 0)
    {
        perror("Accept failed");
        return 1;
    }
        
    return 0;
}

void printList(string& data, int clientIndex)
{
    int onlineNum = 0;
    for(int i = 0; i < clientList.size(); i++)
    {
        if(clientList[i].loggedIn == true)
        {
            onlineNum++;
            data.append(clientList[i].name + "#" + clientList[i].IPAddress + "#");
            char buffer[10] = {0};
            sprintf(buffer, "%d", clientList[i].portNumber);
            data = data + buffer + "\n";
        }
    }
    char buffer1[10] = {0}, buffer2[10] = {0};
    sprintf(buffer1, "%d", clientList[clientIndex].accountBalance);
    sprintf(buffer2, "%d", onlineNum);
    string temp = buffer1;
    temp.append("\n");
    temp.append(buffer2);
    temp.append("\n");
    data = temp + data;
    return;
}

/*
 * This will handle connection for each client
 * */
void* connection_handler(void* socket_desc)
{
    // Get the socket descriptor
    int sock = *(int*)socket_desc;
    int read_size;
    char client_message[2000] = {0};
    int clientIndex = -1;
    string currentClientIP = clientIP;
    int socketIndex = currentSocketIndex;
            
    // Receive a message from client
    while((read_size = recv(sock, client_message, 2000, 0)) > 0)
    {
        string receivedMessage = client_message;
        cout << "Message received by server is:" << receivedMessage << endl;
        size_t cut = receivedMessage.find('#');
        string function;
        if(cut != string::npos)
        {
            function.assign(receivedMessage, 0, cut);
        }
        else
        {
            function.assign(receivedMessage);
        }
        
        string data;
        if(function == "REGISTER")
        {
            string name;
            name.assign(receivedMessage, cut + 1, receivedMessage.size() - cut - 1);
            cout << "The registered name is:" << name << endl;
            bool alreadyRegistered = false;
            int i = 0;
            for(; i < clientList.size(); i++)
            {
                if(clientList[i].name == name)
                {
                    data = "210 FAIL (Account name already registered)\n";
                    alreadyRegistered = true;
                    break;
                }
            }
            if(!alreadyRegistered)
            {
                data = "100 OK\n";
                clientInfo newClient(name);
                clientList.push_back(newClient);
            }
        }
        else if(function == "List")
        {
            if(clientIndex == -1)
            {
                data = "Please log in first to get list.\n";
            }
            else
            {
                printList(data, clientIndex);
            }
        }
        else if(function == "Exit")
        {
            int i = 0;
            for(; i < 3; i++)
            {
                if(*(clientSockList[i].new_sock) == sock)
                {
                    clientSockList[i].used = false;
                    clientSockList[i].new_sock = nullptr;
                    break;
                }
            }
            if(clientIndex != -1)
            {
                clientList[clientIndex].loggedIn = false;
                string emptyStr;
                clientSockList[i].name = emptyStr;
            }
            data = "Bye\n";
        }
        else // login or transfer
        {
            bool flag = true; // true if transfer and false if login
            cut = receivedMessage.find('#', cut + 1);
            if(cut == string::npos) // login
            {
                flag = false;
                bool alreadyRegistered = false;
                string temp;
                temp.assign(receivedMessage, receivedMessage.find('#') + 1);
                for(int i = 0; i < clientList.size(); i++)
                {
                    if(clientList[i].name == function)
                    {
                        alreadyRegistered = true;
                        if(clientList[i].loggedIn == true)
                        {
                            data = "220 AUTH_FAIL (Already logged in elsewhere)\n";
                            break;
                        }
                        clientIndex = i;
                        clientList[i].loggedIn = true;
                        clientList[i].IPAddress = currentClientIP;
                        clientList[i].portNumber = stoi(temp, nullptr);
                        clientSockList[socketIndex].name = function;
                        printList(data, clientIndex);
                        break;
                    }
                }
                if(!alreadyRegistered)
                {
                    data = "220 AUTH_FAIL (Account not registered yet)\n";
                }
            }
            else // transfer
            {
                string payerName = function;
                string payeeName;
                payeeName.assign(receivedMessage, cut + 1);
                string temp;
                temp.assign(receivedMessage, payerName.size() + 1, cut - payerName.size() - 1);
                int transferAmount = stoi(temp, nullptr);
                bool success = false, invalidAmount = false, insufBalance = false, notOnline = false, selfTransfer = false;
                if(transferAmount <= 0)
                {
                    data = "Transfer Fail (Invalid transfer amount)\n";
                }
                else if(payerName == payeeName)
                {
                    data = "Transfer Fail (Transfering to yourself)\n";
                }
                else if(clientList[clientIndex].name == payeeName)
                {
                    for(int i = 0; i < clientList.size(); i++)
                    {
                        if(clientList[i].name == payerName)
                        {
                            if(clientList[i].loggedIn == true && clientList[i].accountBalance >= transferAmount)
                            {
                                success = true;
                                clientList[i].accountBalance -= transferAmount;
                                clientList[clientIndex].accountBalance += transferAmount;
                            }
                            else if(clientList[i].accountBalance < transferAmount)
                            {
                                data = "Transfer Fail (Insuffient Balance)\n";
                            }
                            else
                            {
                                data = "Transfer Fail (Payer not online)\n";
                            }
                            break;
                        }
                    }
                }
                
                if(success)
                {
                    data = "Transfer OK!\n";
                }
                
                for(int i = 0; i < 3; i++)
                {
                    if(clientSockList[i].name == payerName)
                    {
                        const char* message = data.c_str();
                        write(*(clientSockList[i].new_sock), message, strlen(message));
                    }
                }
                
                function = "Transfer";
            }
        }
        
        if(function != "Transfer")
        {
            const char* message = data.c_str();
            write(sock, message, strlen(message));
        }
        
        for(int i = 0; i < 2000; i++)
        {
            client_message[i] = 0;
        }
    }
    
    if(read_size == 0)
    {
        puts("Client disconnected");
        clientCount--;
        clientSockList[socketIndex].used = false;
        fflush(stdout);
    }
    else if(read_size == -1)
    {
        perror("recv failed");
    }
        
    // Free the socket pointer
    free(socket_desc);
    
    return 0;
}
