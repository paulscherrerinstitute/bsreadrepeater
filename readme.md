# BSREAD repeater

Receive from configured sources and replay all messages on the configured output sockets.
A single source can be replayed on multiple outputs.
Input sources and output sockets can be added and removed at runtime.


# Project Goals

✓ Stable and predictable memory and cpu usage.

✓ Tested under Valgrind with real bsread sources.

✓ Configure attached sources and outputs at runtime and startup.

✓ Maintain basic throughput statistics and protocol error counters.

✓ Parse bsread json headers and maintain event rate moving averages per channel.

✓ Block sources with too high event rate (free-running sources) and re-enable periodically.

• Block sources with bsread protocol errors.


# Binaries for AMD64 - RHEL 7

<https://data-api.psi.ch/distri/bsrep-0.4.4-amd64-rhel7.tgz>


# Run the application

Example:
```
./bsrep-0.4.4-amd64-rhel7/bsrep <COMMAND_SOCKET_ADDRESS> [STARTUP_COMMAND_FILE]
```

Configuration is done by sending zmq messages to the command socket.
The command socket is a REP socket on `COMMAND_SOCKET_ADDRESS`.

An optional `STARTUP_COMMAND_FILE` can be specified and those commands are applied at start.
The command file contains one command per line.
Example command file:
```
add-source,tcp://source.psi.ch:9900
add-output,tcp://source.psi.ch:9900,tcp://0.0.0.0:4000
add-output,tcp://source.psi.ch:9900,tcp://0.0.0.0:4001
add-source,tcp://source.psi.ch:9901
add-output,tcp://source.psi.ch:9901,tcp://0.0.0.0:4002
```
For a realistic number of sources, the command file is meant to be
generated by tooling, not by hand.

## JSON commands

The command interface is transitioned to json commands. The commands `add-source` and `add-output`
are available as JSON form, more to follow. JSON commands must be a single-line JSON object
("newline-delimited-json").

Example:
```
{ "kind": "add-source", "source": "tcp://source.psi.ch:9900" }
{ "kind": "add-output", "source": "tcp://source.psi.ch:9900", "output": "tcp://0.0.0.0:4000" }
```


# Commands

All commands are accessible by sending a zmq message to the `REP` socket at `COMMAND_SOCKET_ADDRESS`.


## Add a source

`add-source,<SOURCE>` where `<SOURCE>` is e.g. "tcp://source.psi.ch:9999".

This lets the repeater maintain a connection to the source.
Messages will be always pulled from the source, even when no outputs are created yet
or none of the outputs has a connected client.

```
{ "kind": "add-source", "source": "<SOURCE>" }
```

#### For images, increase max-msg-size

By default, the maximum incoming message size is limited to 20 MB.
Images could require a larger message size.
This can be changed via the `msgmax` parameter:
```
{ "kind": "add-source", "source": "<SOURCE>", "msgmax": 60000000 }
```


## Remove a source

`remove-source,<SOURCE>`

Removes the given source as well as all associated outputs.


## Add output to a source

`add-output,SOURCE,OUTPUT`
where `SOURCE` is e.g. "tcp://source.psi.ch:9999"
and `OUTPUT` e.g. "tcp://0.0.0.0:4321".

Creates a PUSH socket on the given `OUTPUT` where clients can connect to.

Also possible as single-line json:
```
{ "kind": "add-output", "source": "SOURCE", "output": "OUTPUT" }
```


### Add output to a source with zmq queue parameters

This form allows to set the zmq `SNDHWM` and `SNDBUF` values for the output push socket.

It is a JSON command on a single line, e.g.:

`{"kind": "add-output", "source": "SOURCE", "output": "OUTPUT", "sndhwm": 200, "sndbuf": 80000}`


## Remove output from a source

`remove-output,SOURCE,OUTPUT`


## List configured sources and their outputs

`list-sources`


## Global statistics

`stats`

Reply contains among others: total number of received and send messages and bytes.


## Statistics for a specific source and its outputs

`stats-source,SOURCE`

Reply contains received and sent messages for that source and its outputs, as well as
the current set of channels on that source.


## Exit the repeater

`exit`


# Planned developments

* Provide more detailed statistics on command.
* Gather basic statistics from the bsread messages passing by.
* Detect bsread protocol errors.
* Detect erratic source behavior, like too high event frequency.
* Check for ordering of pulse ids and timestamps.
* Analyze event frequency for each channel.
