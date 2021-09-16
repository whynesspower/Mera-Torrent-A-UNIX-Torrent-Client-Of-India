
#include <math.h>
#include <string>
#include <iostream>
#include <dirent.h>
#include <vector>
#include <map>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <sstream>
#include <openssl/sha.h>
#include "Definations.h"
using namespace std;

string command_string = "";
string fileUploadPath = "";
string fileUploadPathGroup = "";
string fileDownloadPath = "";
string fileDownloadName = "";

map<int, file_data_base> files_downloaded;
map<int, string> fileBitVectors;		  //cointains fileID and bit vector
map<int, vector<peer>> currentSeederList; //cointains fileID and information on peer

bool flag_if_login = false;
bool LOGIN_ID = "";
bool flag_peer_seeder = false;
int listenSocket;
int trackerSocket;
const int MAX_CONNECTIONS = 5;
struct sockaddr_in trackerAddress, peerAddress;

void share_piece(string ip, int port, int acc, string filePath, int startPiece, int endPiece)
{
	ifstream fp(filePath, ios::in | ios::binary);
	struct stat sb;
	unsigned long long total_size = stat(filePath.c_str(), &sb);
	total_size = (total_size == 0 ? sb.st_size : 0ll);
	int chunks = (int)::ceil(total_size / (PIECE_SIZE));

	//No of 512 KB chunks in file are chunks
	//Total file size in (bytes) is total_size

	size_t CHUNK_SIZE = PIECE_SIZE;
	if (total_size < CHUNK_SIZE)
		CHUNK_SIZE = total_size;
	if (startPiece != 0)
		total_size = total_size - startPiece * PIECE_SIZE;
	char *buff = new char[CHUNK_SIZE];
	int rer = 0;
	fp.seekg((startPiece)*PIECE_SIZE, ios::beg);

	while (fp.tellg() <= endPiece * PIECE_SIZE && fp.read(buff, CHUNK_SIZE))
	{
		total_size -= CHUNK_SIZE;
		int z = send(acc, buff, CHUNK_SIZE, 0);
		//Sending z bytes through the socket
		rer += z;
		if (total_size == 0)
			break;
		if (total_size < CHUNK_SIZE)
			CHUNK_SIZE = total_size;
	}
	cout << fp.tellg() << endl;
	fp.close();
	cout << "Sent " << rer << " bytes to " << ip << ":" << port << endl;
	int sta = close(acc);
	if (sta == 0)
		cout << "Connection closed with " << ip << ":" << port << endl;
}

void recieve_piece(int seedSocket, string filePath, int fileid, int pieceLocation)
{
	ofstream fp(filePath, ios::out | ios::binary | ios::app);
	fp.seekp(PIECE_SIZE * pieceLocation, ios::beg);
	int i = 0;
	int n, currPiece = pieceLocation;
	string prevBitVec = fileBitVectors[fileid];
	do
	{
		char *buff = new char[PIECE_SIZE];
		memset(buff, 0, PIECE_SIZE);
		n = read(seedSocket, buff, PIECE_SIZE);
		fp.write(buff, n);
		i += n;
		if (n != 0 && currPiece < prevBitVec.length())
			prevBitVec[currPiece++] = '1';
		// Wrote n bytes
	} while (n > 0);
	cout << "Wrote " << i << " bytes\n";
	fileBitVectors[fileid] = prevBitVec;
	fp.close();
}

//when you are calling checkforconnections while downlaoding the format should be
// " d <FILEID> <PIECE RANGE START> <PIECE RANGE END> "

void checkForConnections()
{
	listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	int reuseAddress = 1;
	int listenSocketOptions = setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &reuseAddress, sizeof(reuseAddress));
	setsockopt(listenSocket, SOL_SOCKET, SO_REUSEPORT, &reuseAddress, sizeof(reuseAddress));
	if (listenSocket < 0 || listenSocketOptions < 0)
	{
		cout << "Socket creation error \n";
		return;
	}
	int bindStatus = ::bind(listenSocket, (struct sockaddr *)&peerAddress, sizeof(struct sockaddr_in));
	if (bindStatus < 0)
	{
		cout << ("Bind Failed \n");
		return;
	}
	if (listen(listenSocket, 3) < 0)
	{
		cout << ("Listen Failed \n");
		return;
	}
	char ip[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(peerAddress.sin_addr), ip, INET_ADDRSTRLEN);
	int port = ntohs(peerAddress.sin_port);
	printf("Listen started on %s:%d\n", ip, port);
	socklen_t addr_size = sizeof(struct sockaddr_in);
	while (flag_peer_seeder)
	{
		memset(ip, 0, INET_ADDRSTRLEN);
		struct sockaddr_in clientAddress;
		int acc = accept(listenSocket, (struct sockaddr *)&clientAddress, &addr_size);
		inet_ntop(AF_INET, &(clientAddress.sin_addr), ip, INET_ADDRSTRLEN);
		port = ntohs(clientAddress.sin_port);
		string fullAddress = string(ip) + ":" + to_string(port);
		printf("connection established with peer IP : %s and PORT : %d\n", ip, port);
		char tmp[4096] = {0};
		recv(acc, tmp, sizeof(tmp), 0);
		string req = string(tmp);
		if (req[0] == 'd')
		{
			stringstream x(req);
			string t;
			vector<string> argsFromPeer;
			while (getline(x, t, ' '))
				argsFromPeer.push_back(t);

			printf("Peer %s:%d requested for %s with piece range from %s-%s\n", ip, port, files_downloaded[stoi(argsFromPeer[1])].path.c_str(), argsFromPeer[2].c_str(), argsFromPeer[3].c_str());
			thread sendDataToPeer(share_piece, string(ip), port, acc, files_downloaded[stoi(argsFromPeer[1])].path, stoi(argsFromPeer[2]), stoi(argsFromPeer[3]));
			sendDataToPeer.detach();
		}
		else
			cout << "Invalid command\n";
	}
	cout << "Listen stopped\n";
}

bool getFrequency(string x)
{
	int no1s = 0;
	for (int i = 0; i < x.length(); i++)
	{
		if (x[i] == '1')
			no1s++;
	}
	if (no1s == x.length())
		return false;
	return true;
}

void dowload_start(int fileid, string fileName, string filePath)
{
	vector<peer> peerlist = currentSeederList[fileid];
	int reuseAddress = 1;
	int maxConns = (MAX_CONNECTIONS < peerlist.size() ? MAX_CONNECTIONS : peerlist.size());

	ifstream fp(fileName, ios::in | ios::binary);
	struct stat sb;
	unsigned long long total_size = stat(fileName.c_str(), &sb);
	total_size = (total_size == 0 ? sb.st_size : 0ll);
	int chunks = (int)::ceil(total_size / (PIECE_SIZE)) + 1;
	fp.close();
	cout << "CHUNKS " << chunks << endl;
	size_t CHUNK_SIZE = PIECE_SIZE;
	if (total_size < CHUNK_SIZE)
		CHUNK_SIZE = total_size;

	int startPiece = 0, endPiece = chunks / maxConns;
	string bv(chunks, '0');
	fileBitVectors[fileid] = bv;
	ofstream output(filePath, ios::out | ios::binary | ios::app);
	for (int i = 0; i < maxConns; i++)
	{
		struct sockaddr_in seedAddress;
		seedAddress.sin_family = AF_INET;
		seedAddress.sin_port = htons(peerlist[i].port);
		seedAddress.sin_addr.s_addr = inet_addr(peerlist[i].ip.c_str());

		int seedSocket = socket(AF_INET, SOCK_STREAM, 0);
		setsockopt(seedSocket, SOL_SOCKET, SO_REUSEADDR, &reuseAddress, sizeof(reuseAddress));
		setsockopt(seedSocket, SOL_SOCKET, SO_REUSEPORT, &reuseAddress, sizeof(reuseAddress));
		if (seedSocket < 0)
		{
			cout << "Unable to download. Socket creation error \n";
			return;
		}
		int trackerConnectStatus = connect(seedSocket, (struct sockaddr *)&seedAddress, sizeof(seedAddress));
		if (trackerConnectStatus < 0)
		{
			cout << "Tracker connection Failed with seeder IP " << seedAddress.sin_addr.s_addr << " and port " << seedAddress.sin_port << "\n";
			return;
		}
		//d <FILEID> <PIECE RANGE START> <PIECE RANGE END>
		string sss = "d " + to_string(fileid) + " " + to_string(startPiece) + " " + to_string(endPiece);
		cout << "Send DL request to seed " << peerlist[i].ip << ":" << peerlist[i].port << " for file ID " << fileid << endl;
		send(seedSocket, sss.c_str(), sss.length(), 0);

		thread writeToFile(recieve_piece, seedSocket, filePath, fileid, startPiece);
		writeToFile.join();
		startPiece = endPiece + 1;
		endPiece += (chunks / maxConns);
		if (startPiece >= chunks)
			i = maxConns;
		if (endPiece > chunks)
			endPiece = chunks;
		if (i == maxConns - 2)
			endPiece = chunks;
	}
	int y = 1;

	while (getFrequency(fileBitVectors[fileid]))
		;

	cout << "Downloading of file " + fileName + " completed\n";
	string ss = "o " + to_string(fileid);
	char *buffer = new char[4096];
	memset(buffer, 0, 4096);
	send(trackerSocket, ss.c_str(), ss.length(), 0);
	read(trackerSocket, buffer, 4096);
	cout << string(buffer) << endl;

	int totPiece = 0;
	ifstream ifs(fileName, ios::binary);
	string totalHash = "";
	char *piece = new char[PIECE_SIZE], hash[20];

	while (ifs.read((char *)piece, PIECE_SIZE) || ifs.gcount())
	{
		//SHA1(piece, strlen((char *)piece), hash);
		totalHash += string((char *)hash);
		totPiece++;
		memset(piece, 0, PIECE_SIZE);
	}

	file_data_base f(fileid, fileName, fileName, fileUploadPathGroup, totPiece, totalHash, set<peer>());
	files_downloaded[fileid] = f;
	if (!flag_peer_seeder)
	{
		flag_peer_seeder = true;
		thread startListenOnPeer(checkForConnections);
		startListenOnPeer.detach();
	}
}

int extractCommand()
{
	string s, t;
	getline(cin, s);
	if (s == "" || s == "\n")
		return 0;
	stringstream x(s);
	vector<string> cmds;
	while (getline(x, t, ' '))
	{
		cmds.push_back(t);
	}
	if (cmds[0] == "signup")
	{
		if (cmds.size() < 3)
		{
			cout << "Not enough parameters, You  probably forgot to pass some parameter \n";
			return 0;
		}
		command_string = "a " + cmds[1] + " " + cmds[2];
	}
	else if (cmds[0] == "login")
	{
		if (flag_if_login)
		{
			cout << "Already logged in.\n";
			return 0;
		}
		if (cmds.size() < 3)
		{
			cout << "Not enough parameters, You  probably forgot to pass some parameter \n";
			return 0;
		}
		command_string = "b " + cmds[1] + " " + cmds[2];
		return 10;
	}
	else if (cmds[0] == "create_group")
	{
		if (!flag_if_login)
		{
			cout << "mere bhai log in toh kar pehle! (Traslation: Please log in first)\n";
			return 0;
		}
		if (cmds.size() < 2)
		{
			cout << "Not enough parameters, You  probably forgot to pass some parameter \n";
			return 0;
		}
		command_string = "c " + cmds[1];
	}
	else if (cmds[0] == "join_group")
	{
		if (!flag_if_login)
		{
			cout << "mere bhai log in toh kar pehle! (Traslation: Please log in first)\n";
			return 0;
		}
		if (cmds.size() < 2)
		{
			cout << "Not enough parameters, You  probably forgot to pass some parameter \n";
			return 0;
		}
		command_string = "d " + cmds[1];
	}
	else if (cmds[0] == "leave_group")
	{
		if (!flag_if_login)
		{
			cout << "mere bhai log in toh kar pehle! (Traslation: Please log in first)\n";
			return 0;
		}
		if (cmds.size() < 2)
		{
			cout << "Not enough parameters, You  probably forgot to pass some parameter \n";
			return 0;
		}
		command_string = "e " + cmds[1];
	}
	else if (cmds[0] == "list_request")
	{
		if (!flag_if_login)
		{
			cout << "mere bhai log in toh kar pehle! (Traslation: Please log in first)\n";
			return 0;
		}
		if (cmds.size() < 2)
		{
			cout << "Not enough parameters, You  probably forgot to pass some parameter \n";
			return 0;
		}
		command_string = "f " + cmds[1];
	}
	else if (cmds[0] == "accept_request")
	{
		if (!flag_if_login)
		{
			cout << "mere bhai log in toh kar pehle! (Traslation: Please log in first)\n";
			return 0;
		}
		if (cmds.size() < 3)
		{
			cout << "Not enough parameters, You  probably forgot to pass some parameter \n";
			return 0;
		}
		command_string = "g " + cmds[1] + " " + cmds[2];
	}
	else if (cmds[0] == "list_group")
	{
		if (!flag_if_login)
		{
			cout << "mere bhai log in toh kar pehle! (Traslation: Please log in first)\n";
			return 0;
		}
		command_string = "h";
	}
	else if (cmds[0] == "list_file")
	{
		if (!flag_if_login)
		{
			cout << "mere bhai log in toh kar pehle! (Traslation: Please log in first)\n";
			return 0;
		}
		if (cmds.size() < 2)
		{
			cout << "Not enough parameters, You  probably forgot to pass some parameter \n";
			return 0;
		}
		command_string = "i " + cmds[1];
	}
	else if (cmds[0] == "upload_file")
	{
		if (!flag_if_login)
		{
			cout << "mere bhai log in toh kar pehle! (Traslation: Please log in first)\n";
			return 0;
		}
		if (cmds.size() < 3)
		{
			cout << "Not enough parameters, You  probably forgot to pass some parameter \n";
			return 0;
		}
		char path[4096] = {0};
		string filePath = "";
		getcwd(path, 4096);
		if (cmds[1][0] != '~')
		{
			filePath = (string(path) + (cmds[1][0] == '/' ? "" : "/") + cmds[1]);
		}
		if (FILE *file = fopen(filePath.c_str(), "r"))
		{
			fclose(file);
			command_string = "j " + filePath + " " + cmds[2];
			//cout << command_string << endl;
			fileUploadPath = filePath;
			fileUploadPathGroup = cmds[2];
			return 20;
		}
		else
		{
			cout << "File not found\n";
			return 0;
		}
	}
	else if (cmds[0] == "download_file")
	{
		if (!flag_if_login)
		{
			cout << "mere bhai log in toh kar pehle! (Traslation: Please log in first)\n";
			return 0;
		}
		if (cmds.size() < 4)
		{
			cout << "Not enough parameters, You  probably forgot to pass some parameter \n";
			return 0;
		}
		command_string = "k " + cmds[1] + " " + cmds[2] + " " + cmds[3];
		fileUploadPathGroup = cmds[1];
		fileDownloadName = cmds[2];
		fileDownloadPath = cmds[3];
		return 30;
	}
	else if (cmds[0] == "logout")
	{
		if (!flag_if_login)
		{
			cout << "mere bhai log in toh kar pehle! (Traslation: Please log in first) but quitting anyway\n";
		}
		command_string = "l";
		return 100;
	}
	else if (cmds[0] == "show_download")
	{
		if (!flag_if_login)
		{
			cout << "mere bhai log in toh kar pehle! (Traslation: Please log in first)\n";
			return 0;
		}
		command_string = "m";
	}
	else if (cmds[0] == "stop_share")
	{
		if (!flag_if_login)
		{
			cout << "mere bhai log in toh kar pehle! (Traslation: Please log in first)\n";
			return 0;
		}
		if (cmds.size() < 3)
		{
			cout << "Not enough parameters, You  probably forgot to pass some parameter \n";
			return 0;
		}
		command_string = "n " + cmds[1] + " " + cmds[2];
	}
	else
	{
		cout << "Invalid command\n";
		return 0;
	}
	return 1;
}

int main(int argc, char **argv)
{
	if (argc < 4)
	{
		cout << "Parameters not provided.Exiting...\n";
		return -1;
	}
	int reuseAddress = 1;
	ifstream trackInfo(argv[3]);
	string ix, px;
	trackInfo >> ix >> px;
	trackInfo.close();
	trackerAddress.sin_family = AF_INET;
	trackerAddress.sin_port = htons(stoi(px));
	trackerAddress.sin_addr.s_addr = inet_addr(ix.c_str());

	peerAddress.sin_family = AF_INET;
	peerAddress.sin_port = htons(stoi(argv[2]));
	peerAddress.sin_addr.s_addr = inet_addr(argv[1]);

	char buffer[4096] = {0};
	trackerSocket = socket(AF_INET, SOCK_STREAM, 0);
	int listenSocketOptions = setsockopt(trackerSocket, SOL_SOCKET, SO_REUSEADDR, &reuseAddress, sizeof(reuseAddress));

	setsockopt(trackerSocket, SOL_SOCKET, SO_REUSEPORT, &reuseAddress, sizeof(reuseAddress));
	if (trackerSocket < 0 || listenSocketOptions < 0)
	{
		cout << "Socket creation error \n";
		return -1;
	}
	int trackerConnectStatus = connect(trackerSocket, (struct sockaddr *)&trackerAddress, sizeof(trackerAddress));
	if (trackerConnectStatus < 0)
	{
		cout << ("Tracker connection Failed \n");
		return -1;
	}
	int valread = read(trackerSocket, buffer, 4096);
	cout << string(buffer) << endl;
	string syncActual = "sync " + string(argv[1]) + " " + string(argv[2]);
	send(trackerSocket, syncActual.c_str(), syncActual.length(), 0);
	while (true)
	{
		memset(buffer, 0, 4096);
		int cmdFlag = extractCommand();
		if (cmdFlag == 0 || command_string == "")
			continue;

		send(trackerSocket, command_string.c_str(), (command_string).length(), 0);
		command_string = "";
		valread = read(trackerSocket, buffer, 4096);
		cout << string(buffer) << endl;

		if (cmdFlag == 100)
		{
			flag_peer_seeder = false;
			flag_if_login = false;
			break;
		}
		else if (cmdFlag == 10 && string(buffer).substr(0, 3) == "Log")
		{
			flag_if_login = true;
			//flag_peer_seeder = false;
		}
		else if (cmdFlag == 20 && isdigit(string(buffer)[0]))
		{
			string fileDetails = string(buffer);
			fileDetails = fileDetails.substr(0, fileDetails.find(" "));

			int totPiece = 0;
			ifstream ifs(fileUploadPath, ios::binary);
			string totalHash = "";
			char *piece = new char[PIECE_SIZE], hash[20]; //change array to unsigned for SHA1

			while (ifs.read((char *)piece, PIECE_SIZE) || ifs.gcount())
			{
				totalHash += string((char *)hash);
				totPiece++;
				memset(piece, 0, PIECE_SIZE);
			}

			file_data_base f(stoi(fileDetails), fileUploadPath, fileUploadPath, fileUploadPathGroup, totPiece, totalHash, set<peer>());
			files_downloaded[stoi(fileDetails)] = f;
			if (!flag_peer_seeder)
			{
				flag_peer_seeder = true;
				thread startListenOnPeer(checkForConnections);
				startListenOnPeer.detach();
			}
		}
		else if (cmdFlag == 30 && isdigit(string(buffer)[0]))
		{
			string b = string(buffer);
			//cout << "ORIG " << b << endl;
			stringstream x(b);
			string t;
			vector<peer> peerList;
			int i = 0, fileid = 0;
			while (getline(x, t, ' '))
			{
				if (i == 0)
					fileid = stoi(t);
				else
				{
					peer ppp(t.substr(0, t.find(":")), stoi(t.substr(t.find(":") + 1)), "");
					peerList.push_back(peer(ppp));
					cout << "Added seed " << ppp.ip << ":" << ppp.port << endl;
				}
				i++;
			}
			currentSeederList[fileid] = peerList;
			//START DOWNLOAD
			thread startdl(dowload_start, fileid, fileDownloadName, fileDownloadPath);
			startdl.detach();
		}
	}

	return 0;
}
