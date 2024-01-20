# Multithreading-Chatting-Server (C++)


                              
                                                          
#   Project Description
Building a command-line chat tool using c++ . The tool can be used to communicate with multiple users on a local network. This project will be useful for anyone 
who wants to learn and implement socket programming as well as for anyone who wants to understand and build applications using client-server architecture.

#   Project Stages
1. learning basics concepts of object oriented programming (i.e constructor , deconstructor , intheritence  polymorphism , abstraction)         
2. how to use socket .
3. implementing a simple client-server chat tool in c++ .
4. then use multithreading to make it bidirectional c++ .
5. handling multiple chat rooms functionality into the chat tool.
6. using semaphores , mutex to remove critical state.


# Socket Programming
Sockets are end point of communication . Sockets allow communication between two different processes on the same or different
machines. To be more precise, it's a way to talk to other computers using standard Unix file descriptors. In Unix, every 
I/O action is done by writing or reading a file descriptor. A file descriptor is just an integer associated with an open
file and it can be a network connection, a text file, a terminal, or something else. To a programmer, a socket looks and
behaves much like a low-level file descriptor. This is because commands such as read() and write() work with sockets in
the same way they do with files and pipes. 

# Multithreading
A thread is a light-weight smallest part of a process that can run concurrently with the
other parts(other threads) of the same process. Threads are independent because they all
have separate paths of execution. All threads of a process share the common memory. The
process of executing multiple threads simultaneously is known as multithreading


# Building a chat application
In this milestone i developed a simple chat application ans extend the previously built
client-server program to allow server to accept connections from multiple clients. Here also ,
allow the server the to accept messages from all the client and broadcast the messages to
all the clients.

# Extending the chat tool
extend the functionality of the previous tool so it supports multiple chat rooms. The server
should be able to manage multiple users in multiple chat rooms at a given time.




  # TO USE IT 
  
  1. build a file client.cpp in visual studio . 
  2. build a file server.cpp
  3. Run a server by running server.exe
  4. then start clients.exe file using (client.exe "127.0.0.1" ) 127.0.0.1 is a local server for machine 
  5. then enjoy chatting 

  












