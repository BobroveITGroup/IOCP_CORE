# IOCP\_CORE — High-Performance Asynchronous Server Core Based on Windows IOCP

![License](https://img.shields.io/badge/license-MIT-green.svg)

---

## Overview

**IOCP\_CORE** is a central module for building high-performance, multithreaded servers on Windows using IOCP (Input/Output Completion Ports).
Designed to create scalable TCP/UDP servers with asynchronous client handling, customizable protocols, and efficient client state management.

---

## Features

* Uses Windows IOCP for maximum scalability and performance
* Supports both TCP and UDP protocols
* Thread-safe client management with unique client IDs
* Pipeline model for processing client operations (RECV/SEND)
* Operation pooling to minimize memory allocations
* Uses smart pointers (`std::shared_ptr`) for client lifetime management
* Flexible protocol system allowing custom implementations
* Thread-safe synchronization primitives (mutexes)
* Support for background tasks and periodic timer ticks
* Easily extensible architecture

---

## Components

| Component               | Description                                    |
| ----------------------- | ---------------------------------------------- |
| `IOCP_Client`           | Client structure holding sockets and state     |
| `IOCP_Operation`        | Asynchronous I/O operation                     |
| `IOCP_OperationManager` | Operation pool and manager                     |
| `IOCP_Protocol`         | Abstract protocol interface                    |
| `IOCP_CORE`             | Core server class managing threads and clients |

---

## Quick Start

### Initialize Server

```cpp
auto shokuda_protocol = std::make_shared<ShoKudaProtocol>();

IOCP_CORE server({
    { 12345, 1, shokuda_protocol }
});

server.Run();
```

### Create Custom Client and Protocol

Inherit from `IOCP_Client` to add custom client data, implement `IOCP_Protocol` to handle your business logic.

---

## Architecture

* Creates an I/O Completion Port (IOCP) and listening sockets for TCP/UDP at startup
* Spawns worker threads that process completion events via `GetQueuedCompletionStatus`
* Calls `AcceptClient` on new connections to create client objects
* Performs asynchronous reads/writes using `WSARecv`/`WSASend`
* Protocols implement data processing logic in `ProcedureClient`
* Calls `DeleteClient` when a client disconnects to clean up resources

---

## Implementation Details

* Uses `std::shared_ptr` smart pointers for safe and automatic client lifetime management
* Operation pool (`IOCP_OperationManager`) reduces memory allocation overhead
* Thread-safe access to client data with mutex locking
* Background tasks implemented via a dedicated thread and `OnBackgroundTick` callback
* Easily integrates with any custom protocols and business logic

---

## Requirements

* Windows 10 or later
* C++17 compatible compiler (MSVC recommended)
* Dependencies: Winsock2, Windows API

---

## License

This project is licensed under the MIT License. See LICENSE for details.

---

## Contact

Author: Mykhailo Bobrov

---

## Acknowledgments

Thanks to everyone who contributed, tested, and inspired the creation of this system!

---

If you want, I can also help generate `.gitignore` files, CI/CD configs, or client examples in various languages.

---

Would you like me to prepare a ready-to-upload Markdown file for GitHub?
