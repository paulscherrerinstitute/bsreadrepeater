# BSREAD repeater

Receive from configured sources and replay all messages on the configured output sockets.
A single source can be replayed on multiple outputs.
Input sources and output sockets can be added and removed at runtime.


# Run the application

Currently no command line options are available.

```
./bsrep
```

Configuration is done by sending zmq messages to the command socket.
The command socket is a REP socket on tcp 127.0.0.1:4242.


# Add a source

Send via a zmq REQ socket the message "add-source,SOURCE" to the command socket
where `SOURCE` is e.g. "tcp://source.psi.ch:9999".


# Remove a source

Send via a zmq REQ socket the message "remove-source,SOURCE" to the command socket.


# Add output to a source

Send via a zmq REQ socket the message "add-output,SOURCE,OUTPUT" to the command socket
where `SOURCE` is e.g. "tcp://source.psi.ch:9999" and `OUTPUT` e.g. "tcp://127.0.0.1:4321".


# Remove output from a source

Send via a zmq REQ socket the message "remove-output,SOURCE,OUTPUT" to the command socket.


# Exit the repeater

Send via a zmq REQ socket the message "exit" to the command socket.


# Planned improvements

* Set the command socket port via command-line parameter.
* Provide more detailed statistics on command.
* Gather basic statistics from the bsread messages passing by.
