#ifndef PIPED_PROCESS
#define PIPED_PROCESS

int twoPipesExec(const char* cmd, int& inpipe_fd, int& outpipe_fd);
int readTimeout(int fildes, char* buf, int len, double timeout_sec);

class PipedProcess
{
public:
  PipedProcess(const char* cmd);

  /** Write to stdin */
  int write(const char* buf, int buflen);
  /** Read from stdout */
  int read(char* buf, int buflen, double timeout_sec = -1);

  int getStdoutPipe() { return m_inpipe_fd; }
  int getStdinPipe() { return m_outpipe_fd; }
  int getPid() { return m_pid; }
private:
  /** Pipe to the stdin of new process */
  int m_outpipe_fd;
  /** Pipe from the stdout of new process */
  int m_inpipe_fd;
  /** Child process pid */
  int m_pid;
};

#endif // PIPED_PROCESS
