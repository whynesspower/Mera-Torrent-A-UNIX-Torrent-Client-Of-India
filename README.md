# Mera-Torrent
Mera-Torrent is a miniature implementation of Bit-Torrent application using
C++ and its standard network libraries for Unix based Operating systems.
This project involves challenges like Socket and Network programming,
Muti-Threading, Peer-to-Peer file sharing, Linux Server Programming,
Piece-Selection Algorithm for file sharing, OpenSSl library for SHA1
encoding of the file.

**Please read Project-Report file for detailed explanation**

### How to compile and run the software

**Pre-Requisites**

1. You must have Unix based operating system like MacOS or any
version of Linux, if you are using windows make sure you have
WSL library installed to run Linux based system software
2. You need to have the OpenSSL library installed on your computer.
Installing instructions are given at
https://zoomadmin.com/HowToInstall/UbuntuPackage/libssl-dev
3. I have already provided the compiled files for my software if you
are having any problem compiling the original code

### Running the software

 1. tracker.cpp file is to be launched first
Compile the code yourself using ```g++ -pthread -o tracker tracker.cpp``` command
2. Run the tracker file by using ```./tracker tracker_ip_port.txt 1``` command
3. peer.cpp file is to be launched now Compile the code yourself using ```g++ -pthread -o peer peer.cpp```
command
4. Run the peer file by using ```./peer 127.1.1.10 2212 tracker_ip_port.txt 1```
You would have to change the IP and PORT manually for every peer
5. After you have successfully launched the full program you can proceed
as per the video tutorial or the steps given in the “Working and
Procedure” section of this document.
6. The commands to operate the program are listed as follows -

### Commands 
1. Create User account signup “userID” “password”
2. Login login “userID” “password”
3. Create Group creat_group “groupID”
4. Join Group join_group “groupID”
5. Leave Group leave_group “groupID”
6. Pending request list list_request “groupID”
7. Accept join request of a peer accept_request “groupID”
“userID”
8. List all the groups present in tracker list_group
9. List all the files present to download list_file “groupID”
10. Upload File upload_file “FilePath” “groupID”
11. Download File download_file “groupID” “FileName”
“DestinationPathWithNewFileNameAtEnd”
12. Logout logout
13. Stop Share of File stop_share “groupID” “fileName”

## Working of software [ Theory ]

1. This program basically, allows you to share files between peers/users
without involving any use of a server in between. To share a file first
you need to create an account, login to that account, create a group
and accept requests of other peers to join that particular group.
2. After you have more than one member in your group, any one of the
peers can upload the file, whose information is sent to the tracker, the
rest of the peers can approve if they have the same file and become a2
seeder of the file. If some other peer of the group wants to download
the file, a request is sent to the tracker to give the file details and
details of the seeders of the file.
3. The size of the file to be shared is divided into blocks of 512KB and
these blocks are shared from seeders of the file to the peer requesting
the file. Equal number of pieces are downloaded from all the peers and
these chunks of files are finally reassembled into the original file that
was to be shared.
4. At the end, after downloading is completed, the peers start listening to
the connections again, for more requests of file sharing.

Copyright |  Yashraj Shukla
-----------------------
Linkedin | https://www.linkedin.com/in/whynesspower
Project Duration | July 27th to September 16th, 2021

©@Apache License ©
