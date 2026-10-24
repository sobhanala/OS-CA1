# Team Programming Competition Platform

A C++17 client/server project for team-based programming competitions. Participants pair as coders and navigators, exchange code and chat messages, submit solutions, and receive scoreboard updates.

## Architecture

- TCP handles registration, team communication, and submissions.
- UDP distributes broadcast updates.
- `select()` provides event-based socket multiplexing.
- A Python evaluation service checks submissions.

## Build

Use Linux, a C++17 compiler, Make or CMake, Python 3, and jsoncpp development headers.

```sh
make
```

Alternatively:

```sh
cmake -S . -B build
cmake --build build
```

## Run a local session

Start the evaluation service, server, and clients in separate terminals:

```sh
python3 src/evaluation_server.py
./server 8000
./client alice 127.0.0.1 8000 coder
./client bob 127.0.0.1 8000 navigator
```

The evaluation service uses localhost port 65432. Use `chat`, `code`, `help`, and `exit` in the client as described in the [original project guide](docs/ORIGINAL_README.md).

The evaluator executes submitted programs. Run the prototype in an isolated lab environment with trusted submissions. The publication update does not add an execution sandbox or verify a full competition run.

## Credits and source

This is a public portfolio copy of the shared project [psithlar/OS-CA1](https://github.com/psithlar/OS-CA1), prepared for Sobhan Alaeddini's account. Original authorship and existing license notices are retained. See [SOURCE.md](SOURCE.md) for the source revision and publication notes.
