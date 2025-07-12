#pragma once

#include "Include.h"

/*
//  ______   __                                                 __    __                                                                                             __                                                    __  __  __  __
// /\__  _\ /\ \                                               /\ \__/\ \__  __                                                                        __           /\ \                                                  /\ \/\ \/\ \/\ \
// \/_/\ \/ \ \/      ___ ___                        __      __\ \ ,_\ \ ,_\/\_\    ___      __                       ___ ___      __     _ __   _ __ /\_\     __   \_\ \                      ____    ___     ___     ___\ \ \ \ \ \ \ \ \
//    \ \ \  \/     /' __` __`\                    /'_ `\  /'__`\ \ \/\ \ \/\/\ \ /' _ `\  /'_ `\                   /' __` __`\  /'__`\  /\`'__\/\`'__\/\ \  /'__`\ /'_` \                    /',__\  / __`\  / __`\ /' _ `\ \ \ \ \ \ \ \ \
//     \_\ \__      /\ \/\ \/\ \                  /\ \L\ \/\  __/\ \ \_\ \ \_\ \ \/\ \/\ \/\ \L\ \                  /\ \/\ \/\ \/\ \L\.\_\ \ \/ \ \ \/ \ \ \/\  __//\ \L\ \                  /\__, `\/\ \L\ \/\ \L\ \/\ \/\ \ \_\ \_\ \_\ \_\
//     /\_____\     \ \_\ \_\ \_\                 \ \____ \ \____\\ \__\\ \__\\ \_\ \_\ \_\ \____ \                 \ \_\ \_\ \_\ \__/.\_\\ \_\  \ \_\  \ \_\ \____\ \___,_\                 \/\____/\ \____/\ \____/\ \_\ \_\/\_\/\_\/\_\/\_\
//     \/_____/      \/_/\/_/\/_/                  \/___L\ \/____/ \/__/ \/__/ \/_/\/_/\/_/\/___L\ \                 \/_/\/_/\/_/\/__/\/_/ \/_/   \/_/   \/_/\/____/\/__,_ /                  \/___/  \/___/  \/___/  \/_/\/_/\/_/\/_/\/_/\/_/
//                                                   /\____/                                 /\____/
//                                                   \_/__/                                  \_/__/                                                       





                         ____                     ___  ________  ________  ________        ________  ________  ________  _______                
                        /\  __\_                 |\  \|\   __  \|\   ____\|\   __  \      |\   ____\|\   __  \|\   __  \|\  ___ \    
                       /  \/ \___\                \ \  \ \  \|\  \ \  \___|\ \  \|\  \     \ \  \___|\ \  \|\  \ \  \|\  \ \   __/|  
                       \     /___/                 \ \  \ \  \\\  \ \  \    \ \   ____\     \ \  \    \ \  \\\  \ \   _  _\ \  \_|/__ 
                    /\_/     \    \                 \ \  \ \  \\\  \ \  \____\ \  \___|      \ \  \____\ \  \\\  \ \  \\  \\ \  \_|\ \ 
                   /          \____\                 \ \__\ \_______\ \_______\ \__\          \ \_______\ \_______\ \__\\ _\\ \_______\  
               ___/\       _  /    /                  \|__|\|_______|\|_______|\|__|           \|_______|\|_______|\|__|\|__|\|_______|   
              / \/  \     /_\/____/
              \     /     \___\
              /     \_/\  /   /
             /          \/___/
             \  _       /   /
              \/_|     /___/
                 /     \___\
                 \  /\_/___/
                  \/___/

    IOCP_CORE.h - Центральный модуль асинхронного сервера на Windows IOCP
    ------------------------------------------------------------------------------
    Автор: Mykhailo Bobrov
    Проект: ShoKuda / IOCP Delivery Server
    Ядро высокопроизводительной многопоточной системы на базе Windows IOCP.
    ------------------------------------------------------------------------------
    Компоненты:
      - Dispatcher потоков и воркеров
      - Механизм конвейерной обработки клиентов
      - Асинхронная передача данных чанками
      - Структура клиента с состояниями и буферами
      - Очередь операций и бизнес-логика

    Зависимости:
      - Winsock2, Windows.h, std::thread, std::function

    Лицензия: MIT / Propietary (указать)

    Дата начала разработки: 2025-02-20
    Последнее обновление:   2025-07-12

    ▓ Будь добр к системе, освобождай HANDLE'ы ▓
*/

class IOCP_Client {
public:
    SOCKET tcp_socket = INVALID_SOCKET;
    SOCKET udp_socket = INVALID_SOCKET;

    uint64_t client_id = 0;

    int i_Protocol = -1;

    // Буферы
    std::vector<uint8_t> buffer;

    sockaddr_in udp_address{}; // нужен, если получаем UDP от клиента

    // Флаги состояния
    bool is_connected = false;
    bool is_udp_initialized = false;

    // Время последнего пинга, heartbeat
    uint64_t last_activity_timestamp = 0;

    // Мьютекс на всякий случай
    std::mutex client_mutex;

public:
    IOCP_Client(SOCKET tcpSock, uint64_t id) : tcp_socket(tcpSock), client_id(id), is_connected(true) {}

    void Disconnect() {
        std::lock_guard<std::mutex> lock(client_mutex);

        if (tcp_socket != INVALID_SOCKET) {
            closesocket(tcp_socket);
            tcp_socket = INVALID_SOCKET;
        }

        if (udp_socket != INVALID_SOCKET) {
            closesocket(udp_socket);
            udp_socket = INVALID_SOCKET;
        }

        is_connected = false;
    }

    virtual ~IOCP_Client() {
        Disconnect();
    }

    // Установка UDP-адреса клиента, если получен первый пакет
    void SetUdpAddress(const sockaddr_in& addr) {
        std::lock_guard<std::mutex> lock(client_mutex);
        udp_address = addr;
        is_udp_initialized = true;
    }

    sockaddr_in GetUdpAddress() {
        std::lock_guard<std::mutex> lock(client_mutex);
        return udp_address;
    }
};

enum class OperationType {
    NONE,
    RECV,
    SEND
};
class IOCP_Operation {
public:
    OVERLAPPED overlapped{};
    OperationType type = OperationType::NONE;

    WSABUF wsa_buf{};
    std::vector<uint8_t> buffer;

    std::shared_ptr<IOCP_Client> client = nullptr;

    IOCP_Operation(OperationType op_type, size_t buffer_size, std::shared_ptr <IOCP_Client> owner)
        : type(op_type), client(owner) {
        buffer.resize(buffer_size);
        wsa_buf.buf = reinterpret_cast<CHAR*>(buffer.data());
        wsa_buf.len = static_cast<ULONG>(buffer.size());
        ZeroMemory(&overlapped, sizeof(overlapped));
    }

    void Reset(OperationType new_type, size_t new_size, std::shared_ptr<IOCP_Client> new_owner) {
        type = new_type;
        ZeroMemory(&overlapped, sizeof(overlapped));
        buffer.resize(new_size);
        wsa_buf.buf = reinterpret_cast<CHAR*>(buffer.data());
        wsa_buf.len = static_cast<ULONG>(buffer.size());
        client = std::move(new_owner);
    }

    ~IOCP_Operation() = default;
};

class IOCP_OperationManager {
private:
    std::mutex pool_mutex;
    std::vector<IOCP_Operation*> free_operations;

public:
    IOCP_OperationManager() = default;

    ~IOCP_OperationManager() {
        std::lock_guard<std::mutex> lock(pool_mutex);
        for (auto op : free_operations) {
            delete op;
        }
        free_operations.clear();
    }

    // Получить операцию из пула или создать новую
    IOCP_Operation* Acquire(OperationType type, size_t size, std::shared_ptr <IOCP_Client> client) {
        std::lock_guard<std::mutex> lock(pool_mutex);
        if (!free_operations.empty()) {
            IOCP_Operation* op = free_operations.back();
            free_operations.pop_back();
            op->Reset(type, size, client);
            return op;
        }
        return new IOCP_Operation(type, size, client);
    }

    // Вернуть операцию обратно в пул
    void Release(IOCP_Operation* op) {
        if (!op) return;
        std::lock_guard<std::mutex> lock(pool_mutex);
        // Опционально очистить данные
        op->Reset(OperationType::NONE, 4096, nullptr);
        free_operations.push_back(op);
    }

    // Для отладки: сколько свободных операций сейчас в пуле
    size_t FreeCount() {
        std::lock_guard<std::mutex> lock(pool_mutex);
        return free_operations.size();
    }
};


extern IOCP_OperationManager g_operation_manager; // глобальный объект
bool AsyncSend(std::shared_ptr <IOCP_Client> client, const uint8_t* data, size_t size, DWORD flags);
bool AsyncRecv(std::shared_ptr <IOCP_Client> client, size_t buffer_size, DWORD flags);


enum  IOCP_Transport_protocol {TCP = 32142341234, UDP = 48329489234};
class IOCP_Protocol {
public:
    IOCP_Transport_protocol t_protocol;

    // Вызывается при подключении клиента
    virtual std::unique_ptr<IOCP_Client> AcceptClient(SOCKET client_socket) = 0;

    // Вызывается при завершении асинхронной операции (RECV/SEND)
    virtual int ProcedureClient(std::shared_ptr <IOCP_Client> client, IOCP_Operation* operation, DWORD bytesTransferred) = 0;

    // Вызывается при отключении клиента
    virtual int DeleteClient(std::shared_ptr <IOCP_Client> client) = 0;

    virtual ~IOCP_Protocol() = default;
};

class IOCP_CORE {
public:
    struct PortProtocolPair {
        uint16_t port;
        int protocol_id;
        std::shared_ptr<IOCP_Protocol> protocol_ptr;  // теперь shared_ptr
    };

private:
    std::unordered_map<int, std::shared_ptr<IOCP_Protocol>> protocols; // shared_ptr
    std::unordered_map<uint16_t, SOCKET> port_sockets;
    std::unordered_map<uint64_t, std::shared_ptr<IOCP_Client>> clients;
    std::mutex clients_mutex;
    HANDLE completion_port = nullptr;

    std::vector<std::thread> worker_threads;
    std::thread background_thread;
    bool running = false;
    std::mutex core_mutex;

    std::vector<PortProtocolPair> port_protocol_pairs;

public:
    void StartAcceptLoops() {
        for (const auto& pp : port_protocol_pairs) {
            if (pp.protocol_ptr->t_protocol == IOCP_Transport_protocol::TCP) {
                // Найти сокет по порту
                auto it = port_sockets.find(pp.port);
                if (it == port_sockets.end()) continue;

                SOCKET listen_sock = it->second;
                auto protocol = pp.protocol_ptr;
                int protocol_id = pp.protocol_id;

                std::thread([this, protocol, listen_sock, protocol_id]() {
                    while (IsRunning()) {
                        sockaddr_in client_addr{};
                        int addr_len = sizeof(client_addr);

                        SOCKET client_sock = accept(listen_sock, (sockaddr*)&client_addr, &addr_len);
                        if (client_sock == INVALID_SOCKET) {
                            int err = WSAGetLastError();
                            if (err == WSAEINTR || err == WSAEWOULDBLOCK || !IsRunning()) break;
                            std::cerr << "Accept failed with error: " << err << std::endl;
                            continue;
                        }

                        try {
                            // Принимаем клиента один раз и создаем shared_ptr из unique_ptr
                            std::unique_ptr<IOCP_Client> new_client = protocol->AcceptClient(client_sock);
                            if (!new_client) {
                                closesocket(client_sock);
                                continue;
                            }

                            std::shared_ptr<IOCP_Client> shared_client = std::move(new_client);

                            {
                                std::lock_guard<std::mutex> lock(clients_mutex);
                                clients[shared_client->client_id] = shared_client;
                            }

                            HANDLE result = CreateIoCompletionPort(
                                reinterpret_cast<HANDLE>(shared_client->tcp_socket),
                                completion_port,
                                (ULONG_PTR)protocol_id,
                                0);

                            if (!result) {
                                std::cerr << "Failed to associate client socket to IOCP" << std::endl;
                                protocol->DeleteClient(shared_client); // передаём shared_ptr
                                continue;
                            }

                            // Запускаем асинхронную работу с клиентом (recv/send)
                            protocol->ProcedureClient(shared_client, nullptr, 0); // передаём shared_ptr
                        }
                        catch (const std::exception& e) {
                            std::cerr << "Exception in accept loop: " << e.what() << std::endl;
                        }
                    }
                    }).detach();
            }
        }
    }


protected:
    int background_tick_interval_ms = 1000;

    virtual void OnBackgroundTick() {}
    virtual void OnStart() {}
    virtual void OnStop() {}

public:
    IOCP_CORE(const std::vector<PortProtocolPair>& port_protocols) : port_protocol_pairs(port_protocols)
    {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            throw std::runtime_error("WSAStartup failed");
        }

        completion_port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
        if (!completion_port) {
            WSACleanup();
            throw std::runtime_error("CreateIoCompletionPort failed");
        }

        for (const auto& pp : port_protocols) {
            protocols[pp.protocol_id] = pp.protocol_ptr;

            SOCKET sock = INVALID_SOCKET;
            if (pp.protocol_ptr->t_protocol == IOCP_Transport_protocol::TCP) {
                sock = CreateListenSocket(pp.port);
            }
            else if (pp.protocol_ptr->t_protocol == IOCP_Transport_protocol::UDP) {
                sock = CreateUdpSocket(pp.port);
            }

            if (sock == INVALID_SOCKET) {
                std::cerr << "Failed to create socket for port " << pp.port << std::endl;
                continue;
            }

            port_sockets[pp.port] = sock;

            HANDLE result = CreateIoCompletionPort(reinterpret_cast<HANDLE>(sock), completion_port, (ULONG_PTR)pp.protocol_id, 0);
            if (!result) {
                std::cerr << "Failed to associate socket to IOCP for port " << pp.port << std::endl;
                closesocket(sock);
                port_sockets.erase(pp.port);
            }
        }
    }

    virtual ~IOCP_CORE()
    {
        Stop();

        for (auto& [port, sock] : port_sockets) {
            closesocket(sock);
        }
        port_sockets.clear();

        if (completion_port) {
            CloseHandle(completion_port);
            completion_port = nullptr;
        }

        WSACleanup();
    }

    void Run(int worker_thread_count = std::thread::hardware_concurrency())
    {
        {
            std::lock_guard<std::mutex> lock(core_mutex);
            running = true;
        }

        OnStart();

        StartAcceptLoops();

        for (int i = 0; i < worker_thread_count; ++i) {
            worker_threads.emplace_back([this]() { WorkerThread(); });
        }

        background_thread = std::thread([this]() {
            while (IsRunning()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(background_tick_interval_ms));
                if (!IsRunning()) break;
                OnBackgroundTick();
            }
            });
    }

    void Stop()
    {
        {
            std::lock_guard<std::mutex> lock(core_mutex);
            if (!running) return;
            running = false;
        }

        OnStop();

        for (size_t i = 0; i < worker_threads.size(); ++i) {
            PostQueuedCompletionStatus(completion_port, 0, 0, nullptr);
        }

        for (auto& t : worker_threads) {
            if (t.joinable()) {
                t.join();
            }
        }
        worker_threads.clear();

        if (background_thread.joinable()) {
            background_thread.join();
        }
    }

protected:
    bool IsRunning() {
        std::lock_guard<std::mutex> lock(core_mutex);
        return running;
    }

private:
    SOCKET CreateListenSocket(uint16_t port)
    {
        SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock == INVALID_SOCKET) return INVALID_SOCKET;

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(port);

        if (bind(sock, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR ||
            listen(sock, SOMAXCONN) == SOCKET_ERROR) {
            closesocket(sock);
            return INVALID_SOCKET;
        }

        return sock;
    }

    SOCKET CreateUdpSocket(uint16_t port)
    {
        SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sock == INVALID_SOCKET) return INVALID_SOCKET;

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(port);

        if (bind(sock, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR) {
            closesocket(sock);
            return INVALID_SOCKET;
        }

        return sock;
    }

    void WorkerThread()
    {
        while (true) {
            DWORD bytesTransferred = 0;
            ULONG_PTR completionKey = 0;
            OVERLAPPED* overlapped = nullptr;

            BOOL result = GetQueuedCompletionStatus(
                completion_port,
                &bytesTransferred,
                &completionKey,
                &overlapped,
                INFINITE);

            {
                std::lock_guard<std::mutex> lock(core_mutex);
                if (!running) break;
            }

            if (!result && overlapped == nullptr) {
                // Ошибка или сигнал остановки
                continue;
            }

            int protocol_id = static_cast<int>(completionKey);
            std::shared_ptr<IOCP_Protocol> protocol;

            {
                std::lock_guard<std::mutex> lock(core_mutex);
                auto it = protocols.find(protocol_id);
                if (it != protocols.end()) protocol = it->second;
            }

            if (!protocol || !overlapped) continue;

            IOCP_Operation* operation = reinterpret_cast<IOCP_Operation*>(overlapped);
            std::shared_ptr<IOCP_Client> client = operation->client; // shared_ptr из операции

            if (!client) {
                // Некорректный клиент, логируем и пропускаем
                continue;
            }

            bool delete_client = false;

            try {
                // Передаём shared_ptr client
                int proc_result = protocol->ProcedureClient(client, operation, bytesTransferred);

                if (proc_result == -1) {
                    delete_client = true;
                }
            }
            catch (const std::exception& e) {
                std::cerr << "Exception in ProcedureClient: " << e.what() << std::endl;
                delete_client = true;
            }
            catch (...) {
                std::cerr << "Unknown exception in ProcedureClient" << std::endl;
                delete_client = true;
            }

            if (delete_client) {
                try {
                    // Передаём shared_ptr client
                    protocol->DeleteClient(client);
                }
                catch (const std::exception& e) {
                    std::cerr << "Exception in DeleteClient: " << e.what() << std::endl;
                }
                catch (...) {
                    std::cerr << "Unknown exception in DeleteClient" << std::endl;
                }
                // Клиент удалится автоматически, если shared_ptr нигде больше не держит его
            }
        }
    }
};