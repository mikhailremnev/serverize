#include "piped_process.h"

#include <sys/prctl.h> // prctl
#include <sys/select.h> // select
#include <stdio.h>
#include <stdlib.h> // exit
#include <string.h> // bzero
#include <unistd.h> // pipe, execl, dup2, write, read
#include <signal.h> // SIG* constants

/**
 * @param[out]  inpipe_fd     File descriptor of pipe to stdin
 * @param[out]  outpipe_fd    File descriptor of pipe from stdout+stderr
 *
 * @return Process PID
 */
int twoPipesExec(const char* cmd, int& inpipe_fd, int& outpipe_fd)
{
  int inpipe_fds[2];
  int outpipe_fds[2];

  pipe(inpipe_fds);
  pipe(outpipe_fds);

  pid_t pid = fork();

  if (pid == 0) {
    //== Child
    dup2(outpipe_fds[0], STDIN_FILENO);
    dup2(inpipe_fds[1], STDOUT_FILENO);
    dup2(inpipe_fds[1], STDERR_FILENO);

    //ask kernel to deliver SIGTERM in case the parent dies
    prctl(PR_SET_PDEATHSIG, SIGTERM);

    execl(cmd, cmd, (char*) NULL);
    // Nothing below this line should be executed by child process. If so, 
    // it means that the execl function wasn't successfull, so lets exit:
    perror("ERROR failed starting child process");
    exit(1);
  }
  //== Parent
  // If the child process will exit unexpectedly, the parent process will
  // obtain SIGCHLD signal that can be handled (e.g. you can respawn the child
  // process).

  // Close unused pipe ends
  close(outpipe_fds[0]);
  close(inpipe_fds[1]);

  // Now, you can write to outpipe_fds[1] and read from inpipe_fds[0] :  
  
  outpipe_fd = outpipe_fds[1];
  inpipe_fd = inpipe_fds[0];

  //== Don't forget to do this on process termination:
  // kill(pid, SIGKILL); //send SIGKILL signal to the child process
  // waitpid(pid, &status, 0);

  return pid;
}

int readTimeout(int fildes, char* buf, int len, double timeout_sec)
{
  bzero(buf, len);

  fd_set readfds;
  FD_ZERO(&readfds);
  FD_SET(fildes, &readfds);
  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = timeout_sec * 1e6;

  int ret = select(fildes+1, &readfds, NULL, NULL, &timeout);
  if (ret < 0) return -1;

  ret = FD_ISSET(fildes, &readfds);
  if (ret <= 0) return -2;

  return read(fildes, buf, len);
}

//==============================================

PipedProcess::PipedProcess(const char* cmd)
{
  m_pid = twoPipesExec(cmd, m_inpipe_fd, m_outpipe_fd);
}

int PipedProcess::write(const char* buf, int buflen)
{
  return ::write(m_outpipe_fd, buf, buflen);
}
int PipedProcess::read(char* buf, int buflen, double timeout_sec)
{
  return ::readTimeout(m_inpipe_fd, buf, buflen, timeout_sec);
}

