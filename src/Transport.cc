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
// Transport layer provides TCP commnincation.

# include "Transport.hh"
# include <iostream>

TransportLayer::TransportLayer()
{
  bzero(&serveraddr, sizeof(serveraddr));
  bzero(&clientaddr, sizeof(clientaddr));
  sockfd = -1;
}

int
TransportLayer::createSocket()
{
  if ( (sockfd = socket(AF_INET,SOCK_STREAM, 0) ) == -1 )
    {
    perror("Cannot create server socket!");
    exit(1);
    }
  return sockfd;
}

void
TransportLayer::bindSocket(int sock, unsigned int port)
{
  // fill in the serveraddr structure and call bind
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons(port);
  
  if (bind (sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) == -1)
    {
      perror("Cannot bind socket!");
      exit(1);
    }
}

void
TransportLayer::listenSocket(int sock, int qlen)
{
  // implement a listen.
  if ( listen( sock, qlen) == -1)
    {
      perror("Cannot listen!");
      exit(1);
    }
}

int
TransportLayer::acceptSocket(int sock)
{
  // implement accept

  int connfd;
  socklen_t len = sizeof(clientaddr);
  
  if ( (connfd = accept(sock, (struct sockaddr *)&clientaddr, &len) ) == -1)
    {
      perror("Cannot accept from socket!");
      exit(1);
    }
  else
      return connfd;
}

void
TransportLayer::closeSocket(int sock)
{
  // close a socket
  close(sock);
}

int
TransportLayer::readSocket(int sock, trq& rq)
{
  // read one trq message from a socket
  int bytes_read;

  if ( (bytes_read = recv(sock,&rq,sizeof(trq),0)) == -1 )
    {
      perror("Cannot recv data from client!");
      exit(1);
    }

  return bytes_read;
}

//Write one trq to the socket
void
TransportLayer::writeSocket(int sock, trq& rq)
{
  if ( send (sock,&rq,sizeof(trq),0) == -1)
    {
      perror("Cannot send data to other side!");
      exit(1);
    }
}

// Connect a socket to a server
void
TransportLayer::connectSocket(int sock, string host, unsigned int port)
{
  clientaddr.sin_family = AF_INET;
  clientaddr.sin_port = htons(port);

  // get the host IP from the name.
  struct hostent *h = gethostbyname(host.c_str());

  if ( h_errno )
    {
      perror("Cannot lookup host!");
      exit(1);
    }
  
  if( inet_pton(AF_INET,h->h_addr_list[0], &clientaddr.sin_addr) < 0 )
    {
      perror("Error executing inet_pton");
      exit(1);
    }

  if ( connect(sock, (struct sockaddr *)&clientaddr, sizeof(clientaddr)) < 0 )
    {
      perror("Cannot connect to host!");
      exit(1);
    }
}
