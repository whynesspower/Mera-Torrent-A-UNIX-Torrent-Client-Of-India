#include <netinet/in.h>
#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <thread>
#include "Definations.h"
#include <openssl/sha.h>
using namespace std;

vector<user> user_data_base;							   // cointains userID and password
vector<group> groups_data_base;							   // cointains groupID and ownerID and membersINFO
map<string, peer> list_of_peers;						   // cointains all the peers for a perticular file
map<string, pending_request_data_base> groups_pending_req; // coitains pending requets in hte group
map<string, vector<file_data_base>> files_in_group;		   // all the sharable files in the group
map<int, file_data_base> file_indexes;					   // cointains ID of files and their propertie, like name, path , group in which they were shared
vector<string> connected_clients;						   // All the clients which are currently connected to the group
int FILE_ID = 1;										   // initilize the file ID of first file as 1

void utility_communication(string ip, int p, int acc)
{
	cout << "Peer has been setup successfully! You can now proceed further\n";
	char *buffer;
	bool flag_if_login = false;
	string LOGIN_ID = "";

	while (true)
	{
		buffer = new char[4096];
		memset(buffer, 0, 4096);
		int l = read(acc, buffer, 4096);
		printf("Client %s:%d said %s (Msg length %d)\n", ip.c_str(), p, buffer, l);
		string recieved_command = string(buffer);
		string t, msg = "";
		stringstream x(recieved_command);
		vector<string> cmds;

		while (getline(x, t, ' '))
		{
			cmds.push_back(t);
		}

		if (cmds[0] == "a")
		{
			user u(cmds[1], cmds[2]);
			bool flag = false;
			for (int i = 0; i < user_data_base.size(); i++)
			{
				if (user_data_base[i].userID == cmds[1])
				{
					msg = "User already exists";
					flag = true;
					break;
				}
			}
			if (!flag)
			{
				msg = "User added";
				user_data_base.push_back(u);
			}
			send(acc, msg.c_str(), msg.length(), 0);
		}
		else if (cmds[0] == "b")
		{
			peer client(ip, p, cmds[1]);
			int i;
			for (i = 0; i < user_data_base.size(); i++)
			{
				if (user_data_base[i].userID == cmds[1] && user_data_base[i].password == cmds[2])
					break;
			}
			if (i == user_data_base.size())
			{
				msg = "User does not exist/Wrong password";
			}
			else
			{
				if (list_of_peers.count(cmds[1]) == 0)
				{
					LOGIN_ID = cmds[1];
					list_of_peers[LOGIN_ID] = client;
					flag_if_login = true;
					msg = "Logging in with user ID " + cmds[1];
				}
				else
				{
					msg = "Already logged in another instance";
				}
			}
			send(acc, msg.c_str(), msg.length(), 0);
		}
		else if (cmds[0] == "c")
		{
			group grp(cmds[1], LOGIN_ID);
			set<string> mems; //add admin to group
			mems.insert(LOGIN_ID);
			grp.members = mems;
			int i;
			bool flag = false;
			for (i = 0; i < groups_data_base.size(); i++)
			{
				if (groups_data_base[i].name == cmds[1])
				{
					flag = true;
					break;
				}
			}
			if (flag)
			{
				msg = "Group already exists";
			}
			else
			{
				groups_data_base.push_back(grp);

				msg = "Group " + grp.name + " created with admin " + grp.adminID;
			}
			send(acc, msg.c_str(), msg.length(), 0);
		}
		else if (cmds[0] == "d")
		{
			int i;
			bool flag = false;
			for (i = 0; i < groups_data_base.size(); i++)
			{
				if (groups_data_base[i].name == cmds[1])
				{

					flag = true;
					break;
				}
			}
			if (!flag)
			{
				msg = "Group not found";
			}
			else
			{
				set<string> existingMembers = groups_data_base[i].members;
				if (existingMembers.find(LOGIN_ID) != existingMembers.end())
					msg = LOGIN_ID + " is already a part of group " + groups_data_base[i].name;
				else
				{
					set<string> xxx = groups_pending_req[cmds[1]].pendingID;
					xxx.insert(LOGIN_ID);
					pending_request_data_base gg(cmds[1], groups_data_base[i].adminID, xxx);
					groups_pending_req[cmds[1]] = gg;
					msg = "Request sent";
				}
			}
			send(acc, msg.c_str(), msg.length(), 0);
		}
		else if (cmds[0] == "e")
		{
			string grpName = cmds[1];
			int i;
			for (i = 0; i < groups_data_base.size(); i++)
			{
				if (groups_data_base[i].name == grpName)
					break;
			}
			if (i == groups_data_base.size())
			{
				msg = "Group not found";
			}
			else
			{
				set<string> grpmembers = groups_data_base[i].members;
				if (grpmembers.find(LOGIN_ID) == grpmembers.end())
					msg = "User " + LOGIN_ID + " not present";
				else
				{
					grpmembers.erase(LOGIN_ID);
					groups_data_base[i].members = grpmembers;
					msg = "User " + LOGIN_ID + " removed from group " + groups_data_base[i].name;
				}
			}
			send(acc, msg.c_str(), msg.length(), 0);
		}
		else if (cmds[0] == "f")
		{
			pending_request_data_base grpp = groups_pending_req[cmds[1]];
			if (grpp.grpname == "")
				msg = "Group not found/No pending requests";
			else
			{
				string pp = "";
				for (auto i = grpp.pendingID.begin(); i != grpp.pendingID.end(); ++i)
					pp += (*i + " ");
				if (pp == "")
					msg = "Group not found/No pending requests";
				else
					msg = "For group " + grpp.grpname + " pending requests are: " + pp;
			}
			send(acc, msg.c_str(), msg.length(), 0);
		}
		else if (cmds[0] == "g")
		{
			pending_request_data_base grpp = groups_pending_req[cmds[1]];
			if (grpp.grpname == "")
				msg = "Group not found/No pending requests";
			else if (grpp.adminname != LOGIN_ID)
				msg = LOGIN_ID + " is not the admin for group " + grpp.grpname;
			else
			{
				set<string> pendID = grpp.pendingID;
				if (pendID.find(cmds[2]) == pendID.end())
					msg = "Group join request for " + cmds[2] + " not found with group " + grpp.grpname;
				else
				{
					pendID.erase(cmds[2]);
					grpp.pendingID = pendID;
					groups_pending_req[cmds[1]] = grpp;
					msg = "For group " + grpp.grpname + ", join request approved for " + cmds[2];
				}
			}
			send(acc, msg.c_str(), msg.length(), 0);
		}
		else if (cmds[0] == "h")
		{
			msg = "All groups in the network: ";
			for (int i = 0; i < groups_data_base.size(); i++)
				msg += ("\n" + groups_data_base[i].name);
			send(acc, msg.c_str(), msg.length(), 0);
		}
		else if (cmds[0] == "i")
		{
			vector<file_data_base> grpFiles = files_in_group[cmds[1]];
			if (grpFiles.size() == 0)
				msg = "No files present in group " + cmds[1];
			else
			{
				msg = "Files present ";
				for (int i = 0; i < grpFiles.size(); i++)
				{
					msg += ("\n" + grpFiles[i].path);
				}
			}
			send(acc, msg.c_str(), msg.length(), 0);
		}
		else if (cmds[0] == "j")
		{
			int i, totPiece = 0;
			string totalHash = "";
			char *piece = new char[PIECE_SIZE], hash[20]; //make unsigned for SHA1
			ifstream ifs(cmds[1], ios::binary);
			while (ifs.read((char *)piece, PIECE_SIZE) || ifs.gcount())
			{
				// SHA1((char *)piece, strlen((char *)piece), (char *)hash);
				totalHash += string((char *)hash);
				totPiece++;
				memset(piece, 0, PIECE_SIZE);
			}
			cout << "HASH done Pieces=" << totPiece << "\n";
			for (i = 0; i < groups_data_base.size(); i++)
			{
				if (groups_data_base[i].name == cmds[2])
					break;
			}
			if (i == groups_data_base.size())
				msg = "Group " + cmds[2] + " not found";
			else
			{
				vector<file_data_base> vv = files_in_group[cmds[2]];
				peer currentPeer = list_of_peers[LOGIN_ID];
				set<peer> peerSet;
				peerSet.insert(currentPeer);
				file_data_base fp(FILE_ID++, cmds[1], cmds[1], cmds[2], totPiece, totalHash, peerSet);
				for (i = 0; i < vv.size(); i++)
				{
					if (vv[i].path == cmds[1])
						break;
				}
				if (i < vv.size())
					msg = "File " + cmds[1] + " already exists in group " + cmds[2];
				else
				{
					vv.push_back(fp);
					files_in_group[cmds[2]] = vv;
					file_indexes[FILE_ID - 1] = fp;
					msg = to_string(FILE_ID - 1) + " ID File " + cmds[1] + " added to group " + cmds[2];
				}
			}
			send(acc, msg.c_str(), msg.length(), 0);
		}
		else if (cmds[0] == "k") // DL FILE
		{
			int i;
			for (i = 0; i < groups_data_base.size(); i++)
			{
				if (groups_data_base[i].name == cmds[1])
					break;
			}
			if (i == groups_data_base.size())
				msg = "Group " + cmds[1] + " not found";
			else
			{
				vector<file_data_base> files = files_in_group[cmds[1]];
				for (i = 0; i < files.size(); i++)
				{
					if (files[i].path.compare(files[i].path.length() - cmds[2].length(), cmds[2].length(), cmds[2]) == 0)
						break;
				}
				if (i == files.size())
					msg = "File " + cmds[2] + " not found in group " + cmds[1];
				else
				{
					set<peer> seeds = file_indexes[files[i].id].seederList; //files[i].seederList;
					if (seeds.empty())
						msg = "No seeds are currently present";
					else
					{
						msg = to_string(files[i].id) + " "; //add file id to beginning
						for (auto i = seeds.begin(); i != seeds.end(); ++i)
							msg += ((*i).ip + ":" + to_string((*i).port) + " ");
					}
				}
			}
			send(acc, msg.c_str(), msg.length(), 0);
		}
		else if (cmds[0] == "l")
		{
			msg = "Logged out. Bye!";
			list_of_peers.erase(LOGIN_ID);
			auto pos = find(connected_clients.begin(), connected_clients.end(), ip + ":" + to_string(p));
			if (pos != connected_clients.end())
				connected_clients.erase(pos);
			send(acc, msg.c_str(), msg.length(), 0);
			break;
		}
		else if (cmds[0] == "m")
		{
			// Show downloads in peer side, nothing to do here
			msg = "File Downloaded 100%";
			send(acc, msg.c_str(), msg.length(), 0);
		}
		else if (cmds[0] == "n")
		{
			msg = "Not implemented";
			send(acc, msg.c_str(), msg.length(), 0);
		}
		else
		{
			msg = "Unknowxn value";
			send(acc, msg.c_str(), msg.length(), 0);
		}
	}
}

int main(int argc, char **argv)
{
	if (argc < 3)
	{
		cout << "Parameters not provided.Exiting...\n";
		return -1;
	}
	ifstream trackInfo(argv[1]);
	string ix, px;
	trackInfo >> ix >> px;
	trackInfo.close();

	int reuseAddress = 1;
	struct sockaddr_in trackerAddress, peerAddress;
	trackerAddress.sin_family = AF_INET;
	trackerAddress.sin_port = htons(stoi(px));
	trackerAddress.sin_addr.s_addr = inet_addr(ix.c_str());
	int addrlen = sizeof(trackerAddress);

	int socketStatus = socket(AF_INET, SOCK_STREAM, 0);
	int listenSocketOptions = setsockopt(socketStatus, SOL_SOCKET, SO_REUSEADDR, &reuseAddress, sizeof(reuseAddress));
	if (socketStatus < 0 || listenSocketOptions < 0)
	{
		cout << "Socket creation error \n";
		return -1;
	}
	int bindStatus = ::bind(socketStatus, (struct sockaddr *)&trackerAddress, sizeof(trackerAddress));
	if (bindStatus < 0)
	{
		printf("Bind failed with status: %d\n", bindStatus);
		return -1;
	}
	int listenStatus = listen(socketStatus, 3);
	if (listenStatus < 0)
	{
		cout << ("Listen Failed \n");
		return -1;
	}

	char ip[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(trackerAddress.sin_addr), ip, INET_ADDRSTRLEN);
	int port = ntohs(trackerAddress.sin_port);
	printf("Succcess! Listen started on %s:%d\n", ip, port);
	socklen_t addr_size = sizeof(struct sockaddr_in);

	while (true)
	{
		int acc = accept(socketStatus, (struct sockaddr *)&peerAddress, &addr_size);
		inet_ntop(AF_INET, &(peerAddress.sin_addr), ip, INET_ADDRSTRLEN);
		port = ntohs(peerAddress.sin_port);

		string fullAddress = string(ip) + ":" + to_string(port);
		printf("Connection Established successfully with IP : %s and PORT : %d\n", ip, port);
		string temp = "Tracker Established successfully! Tracker is  connected to " + ix + ":" + px + " with IP " + string(ip) + ":" + to_string(port);
		send(acc, temp.c_str(), temp.length(), 0);

		char *buffer = new char[4096];
		memset(buffer, 0, 4096);
		int l = read(acc, buffer, 4096);

		string recieved_command = string(buffer);
		string t, msg = "";
		stringstream x(recieved_command);
		vector<string> cmds;

		while (getline(x, t, ' '))
		{
			cmds.push_back(t);
		}
		if (cmds[0] == "sync")
		{
			port = stoi(cmds[2]);
			strcpy(ip, cmds[1].c_str());
			fullAddress = string(ip) + ":" + to_string(port);
			cout << "Actual IP " << string(ip) << ":" << port << "\n";
		}

		if (find(connected_clients.begin(), connected_clients.end(), fullAddress) != connected_clients.end())
			continue;
		else
			connected_clients.push_back(fullAddress);
		thread launchPeer(utility_communication, string(ip), port, acc); //, string(ip), port,descriptor
		launchPeer.detach();
	}
	return 0; //compile with g++ -pthread -o tracker tracker.cpp
}