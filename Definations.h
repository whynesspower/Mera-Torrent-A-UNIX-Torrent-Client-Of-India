#include <iostream>
#include <set>
#include <fstream>
#include <string>
#include <thread>
#include <string.h>
#include <netinet/in.h>
#include <openssl/sha.h>
using namespace std;

size_t PIECE_SIZE = 512 * 1024;
class pending_request_data_base
{
public:
	pending_request_data_base()
	{
		grpname = "";
		adminname = "";
	}
	pending_request_data_base(string gName, string admin, set<string> members)
	{
		grpname = gName;
		adminname = admin;
		pendingID = members;
	}
	pending_request_data_base(const pending_request_data_base &g)
	{
		grpname = g.grpname;
		adminname = g.adminname;
		pendingID = g.pendingID;
	}
	string grpname;
	string adminname;
	set<string> pendingID;
};

class user
{
public:
	user() {}
	user(string userID, string password)
	{
		this->userID = userID;
		this->password = password;
	}
	string userID;
	string password;
};

class peer
{
public:
	peer()
	{
		ip = "";
		port = 0;
		userID = "";
	}
	peer(string ip, int port, string userID)
	{
		this->ip = ip;
		this->port = port;
		this->userID = userID;
	}
	/*peer &operator=(const peer &t)
	{
		this->ip = ip;
		this->port = port;
		this->userID = userID;
	}*/
	bool operator<(const peer &rhs) const
	{
		return port < rhs.port;
	}

	peer(const peer &p)
	{
		ip = p.ip;
		port = p.port;
		userID = p.userID;
	}
	string ip;
	int port;
	string userID;
};

class group
{
public:
	group(string name, string adminID)
	{
		this->name = name;
		this->adminID = adminID;
	}
	string name;
	string adminID;
	set<string> members;
};

class file_data_base
{
public:
	int id;
	string name;
	string path;
	string groupName;
	int pieces;
	set<peer> seederList;
	string hash;

	file_data_base()
	{
		id = -1;
		name = "";
		path = "";
		groupName = "";
		pieces = 0;
		hash = "";
	}
	file_data_base(int i, string n, string p, string grp, int pi, string hh, set<peer> seedList)
	{
		id = i;
		name = n;
		path = p;
		groupName = grp;
		pieces = pi;
		hash = hh;
		seederList = seedList;
	}
	file_data_base(const file_data_base &f)
	{
		id = f.id;
		name = f.name;
		path = f.path;
		groupName = f.groupName;
		pieces = f.pieces;
		hash = f.hash;
		seederList = f.seederList;
	}
};
