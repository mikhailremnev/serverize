#ifndef PIPED_PROCESS
#define PIPED_PROCESS

/**
 * @param[out]  inpipe_fd     File descriptor of pipe to stdin
 * @param[out]  outpipe_fd    File descriptor of pipe from stdout+stderr
 *
 * @return Process PID
 */
int twoPipesExec(char* const* argv, int& inpipe_fd, int& outpipe_fd);
int readTimeout(int fildes, char* buf, int len, double timeout_sec);

class PipedProcess
{
public:
  PipedProcess(char* const cmd);
  /**
   * argv[argc] is guaranteed to be NULL, as per section 5.1.2.2.1 of C standard
   * ("The New C Standard" book), so you can just pass shifted argv from main,
   * if that's possible
   */
  PipedProcess(int argc, char** argv);

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
