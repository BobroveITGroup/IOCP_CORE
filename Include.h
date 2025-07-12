#pragma once

// Windows headers
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <Windows.h>

// Standard headers
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <mutex>
#include <thread>
#include <iostream>
#include <sstream>
#include <atomic>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "mswsock.lib")