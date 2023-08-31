#include <Winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <sstream>

#pragma comment (lib, "ws2_32.lib")

int main()
{
	//Initialize winsock
	WSADATA wsaData;
	int initWinsock = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (initWinsock != 0)
	{
		std::cerr << "Cant Innitialize winsock\n";
		return 1;
	}

	//Create a socket
	SOCKET server = socket(AF_INET, SOCK_STREAM, 0);
	if (server == INVALID_SOCKET)
	{
		std::cerr << "Fail to create socket\n";
		WSACleanup();
		return 1;
	}
	std::cout << "Server Socket Created... \nWaiting for connection...\n";
	std::cout << "Enter Prefered Port: ";
	int portNumber;
	std::cin >> portNumber;

	//Bind Port and IP to socket
	sockaddr_in server_addr, client_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(portNumber);
	server_addr.sin_addr.S_un.S_addr = INADDR_ANY;
	if (bind(server, (sockaddr*)&server_addr, sizeof(server_addr)) == INVALID_SOCKET)
	{
		std::cerr << "Fail to bind socket\n";
		closesocket(server);
		WSACleanup();
			return 1;
	}
	std::cout << "Socket binding successful!!!\nSocket waiting for connections\n";

	//Listening for connections
	listen(server, SOMAXCONN);

	//Creat the fd_set and innitialize to zero
	fd_set masterConnection;
	FD_ZERO(&masterConnection);
	
	//add server to the fd_set
	FD_SET(server, &masterConnection);

	//create a continous loop
	bool running = true;

	while (running)
	{
		//make a copy of the fd_set
		fd_set copyConnection = masterConnection;

		//check who is talking to the server
		int incomming = select(FD_SETSIZE, &copyConnection, nullptr, nullptr, nullptr);

		//iterate the incomming connection to check the listening socket and messages
		for (int i = 0; i < incomming; i++)
		{
			//the array member of the fd_set holds the socket file descriptor of incomming sockets and message
			SOCKET incommingSocket = copyConnection.fd_array[i];
			//check if it is a listening socket
			if (incommingSocket == server)
			{
				//Accept new connection
				int addr_len = sizeof(sockaddr_in);
				SOCKET client = accept(server, nullptr, nullptr);
				if (client == INVALID_SOCKET)
				{

					std::cerr << "Fail to accept connection\n";
					return 1;
				}
				else
				{
					std::cout << "New client " << client << ", accepted\n";

					//add new connection to list of connection in fd_set
					FD_SET(client, &masterConnection);

					//send a welcome message to the new client
					std::string welcomeMessage = "Welcome to the Awesome Chat Server   ";
					send(client, welcomeMessage.c_str(), welcomeMessage.length() +1, 0);
				}
				
			}
			else //it is a message, read the message and broadcast to all clients
			{
				char receiveBuffer[4096];
				ZeroMemory(receiveBuffer, sizeof(receiveBuffer));

				//Receive message
				int bytesReceived = recv(incommingSocket, receiveBuffer, sizeof(receiveBuffer), 0);
				std::cout << "Message Received from " << incommingSocket << " <-> Broadcasting it now.\n";
				if (bytesReceived <= 0)
				{
					//Drop Client
					closesocket(incommingSocket);
					FD_CLR(incommingSocket, &masterConnection);
					std::cout << "Client" << incommingSocket <<  " Droped\n";
				}
				else
				{
					std::string sendBuffer;
					std::cout << "Sending out message: " << receiveBuffer;
					//Send message to other clients
					for (int i = 0; i < masterConnection.fd_count; i++)
					{
						SOCKET sendSock = masterConnection.fd_array[i];
						if (sendSock != server && sendSock != incommingSocket)
						{
							std::ostringstream sout;
							sout << "From " << incommingSocket << " -> " << receiveBuffer << std::endl;
							sendBuffer = sout.str();
							send(sendSock, sendBuffer.c_str(), sendBuffer.length() +1, 0);
						}
					}
				}
			}
		}


	}

	//remove the server from fd_set and close it
	FD_CLR(server, &masterConnection);
	closesocket(server);

	//send a closing message to all the clients
	std::string sendBuffer = "Server is shutting down. \nGoodbye and have a nice day!!!";
	
	while (masterConnection.fd_count > 0)
	{
		//get each socket
		SOCKET closeSocket = masterConnection.fd_array[0];
		//send goodbye message
		send(closeSocket, sendBuffer.c_str(), sendBuffer.length(), 0);
		//remover from fd_set
		FD_CLR(closeSocket, &masterConnection);
		closesocket(closeSocket);

	}
	WSACleanup();



	return 0;
}