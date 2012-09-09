// Copyright (C) 2007 Satya Kiran Popuri <spopur2@uic.edu>
//  
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//  
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//  
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//  
// This class wraps common TCP operations.

# ifndef TRANSPORT_HH
# define TRANSPORT_HH

# include <unistd.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <sys/socket.h>
# include <sys/types.h>
# include <string>
# include <stdio.h>		// for perror
# include <netdb.h>		// for gethostbyname

# include <trq.hh>

using namespace std;

class TransportLayer
{
  int sockfd;			// socket to use.
  struct sockaddr_in serveraddr;
  struct sockaddr_in clientaddr;

public:
  TransportLayer();
  
  int createSocket();
  void bindSocket(int socket, unsigned int port);
  void listenSocket(int socket, int qlen);
  int acceptSocket(int socket);
  void connectSocket(int socket, string host, unsigned int port);
  
  int readSocket(int socket, trq& message);
  void writeSocket(int socket, trq& message);
  void closeSocket (int socket);
};
  
# endif  

  
