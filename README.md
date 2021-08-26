Serverizer
----------

This application aims to provide TCP server to remotely access any interactive shell application.

It uses fork() to start a child process, then uses dup2(..) to link with its stdin and stdout.
After that, any input from the client goes into child process and any output from child process
is sent back to client.

Hence, we get very simple network-accessible services for accessing interactive shells.
(and interactive shells can be easily made from normal C code using [functors](https://github.com/mikhailremnev/functors/) library).

Usage:

    ./serverize [-p port] [-d] command_name [command arguments]

`-p port` is a server port for the client connection. Default is 1234.

If `-d` is specified, the server is started as daemon.

