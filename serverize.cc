#include <string.h>
#include <sys/stat.h> // umask
#include <netinet/in.h> // htons
#include <unistd.h>

#include <sys/select.h> // select
#include <sys/prctl.h> // prctl

#include <errno.h> // errno

#include <signal.h> // SIG* constants

#include <iostream>

#include "piped_process.h"

void error(const char *msg)
{
  perror(msg);
  exit(1);
}

class SocketServer
{
public:
  SocketServer(int port) : port(port)
  {
    server_sock =  socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0)
      error("ERROR opening socket");

    //== Setting SO_REUSEADDR to avoid "Address already in use" error upon restarting server.
    int enable = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
      error("ERROR on setsockopt(SO_REUSEADDR)");

    bzero((char *) &serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;  
    serv_addr.sin_addr.s_addr = INADDR_ANY;  
    serv_addr.sin_port = htons(port);
  }

  void init()
  {
    if (bind(server_sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
       error("ERROR on binding");
  
    // Max number of outstanding connections in the socket connection queue
    const int backlog = 5;
    listen(server_sock, backlog);
  
    clilen = sizeof(cli_addr);
  }

  void accept()
  {
    client_sock = ::accept(server_sock, (struct sockaddr *) &cli_addr, &clilen);

    if (client_sock < 0)
      error("ERROR on accept");

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 1000;
    setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

    int ret;

    while (true) {
      //=== READ FROM CLIENT

      bzero(buffer, 4096);
      int n = read(client_sock, buffer, 4095);
      if (n < 0) {
        // Check if it is not a timeout
        if ((errno != EAGAIN) && (errno != EWOULDBLOCK)) {
          error("ERROR reading from socket");
        }
      }
      if (n == 0) break;
      if (n > 1) {
        // Reroute the message to STDOUT
        write(STDOUT_FILENO, buffer, n);
        fflush(stdout);
        fflush(stdin);
        sleep(0.5);
      }

      //=== READ FROM PROCESS

      bzero(buffer, 4096);
      n = readTimeout(STDIN_FILENO, buffer, 4095, 0.1);

      // TODO: Not sure if that's the correct way.
      // If 0 is returned, process has most likely died.
      if (n == 0) {
        ::close(client_sock);
        close();
        exit(0);
      } else if (n < 0) {
        // Most likely a timeout.
        // TODO: Exit if timeout happens too often?
        continue;
      }
      send(client_sock, buffer, n, 0);
    }

    ::close(client_sock);
  }

  void close()
  {
    ::close(server_sock);
  }

private:
  int server_sock, client_sock, port;
  socklen_t clilen;
  char buffer[4096];
  struct sockaddr_in serv_addr, cli_addr;
};

void startServer(int port)
{
  //== Init server
  SocketServer server(port);
  server.init();

  //== Start serverized process
  PipedProcess proc("/usr/bin/bash");
  dup2(proc.getStdinPipe(), STDOUT_FILENO);
  dup2(proc.getStdoutPipe(), STDIN_FILENO);

  //== Start server
  while (true) server.accept();
}

// From https://stackoverflow.com/questions/17954432/creating-a-daemon-in-linux/17955149
void startDaemon(int port)
{
  pid_t pid;

  /* Fork off the parent process */
  pid = fork();

  /* An error occurred */
  if (pid < 0) exit(EXIT_FAILURE);

  /* Success: Let the parent terminate */
  if (pid > 0) exit(EXIT_SUCCESS);

  /* On success: The child process becomes session leader */
  if (setsid() < 0) exit(EXIT_FAILURE);

  /* Catch, ignore and handle signals */
  //TODO: Implement a working signal handler */
  signal(SIGCHLD, SIG_IGN);
  signal(SIGHUP, SIG_IGN);

  /* Fork off for the second time*/
  pid = fork();

  /* An error occurred */
  if (pid < 0) exit(EXIT_FAILURE);

  /* Success: Let the parent terminate */
  if (pid > 0) exit(EXIT_SUCCESS);

  /* Set new file permissions */
  umask(0);

  /* Change the working directory to the root directory */
  /* or another appropriated directory */
  chdir("/");

  /* Close all open file descriptors */
  // for (int x = sysconf(_SC_OPEN_MAX); x>=0; x--) close (x);

  /* Redirect stderr to /dev/null */
  freopen("/dev/null", "w+", stderr);

  /* Open the log file */
  // openlog ("firstdaemon", LOG_PID, LOG_DAEMON);
  
  startServer(port);
}

// Init socket server.
int main(int argc, char** argv)
{
  bool daemon = false;
  int port = 1234;

  if (argc >= 2) {
    for (int i = 1; i < argc; i++) {
      std::string arg = argv[i];
      if (arg == "-d") {
        daemon = true;
      } else if (arg == "-p") {
        i++;
        port = atoi(argv[i]);
      } else {
        printf("Usage: %s [-p port] [-d]\n", argv[0]);
        return 1;
      }
    }
  }

  if (daemon) {
    startDaemon(port);
  } else {
    startServer(port);
  }

  return 0;
}

