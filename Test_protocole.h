#pragma once

#include "Include.h"
#include "IOCP_CORE.h"

class TestClientContext : public IOCP_Client {
public:
    bool is_echo_mode = true;

    TestClientContext(SOCKET tcpSock, uint64_t id)
        : IOCP_Client(tcpSock, id) {}
};

class TestProtocol : public IOCP_Protocol {
public:
    TestProtocol() { t_protocol = TCP; }

    int AcceptClient(IOCP_Client* client) override {
        std::cout << "[TestProtocol] Accept client ID=" << client->client_id << "\n";
        return AsyncRecv(client,2000,0); // сразу начинаем приём
    }

    int ProcedureClient(IOCP_Client* client, IOCP_Operation* operation, DWORD bytesTransferred) override {
        if (operation->type == OperationType::RECV) {
            std::cout << "[TestProtocol] Received " << bytesTransferred << " bytes from client ID=" << client->client_id << "\n";
            // эхо-ответ
            AsyncSend(client, operation->buffer.data(), bytesTransferred,0);
            // снова начинаем приём
            AsyncRecv(client,2000,0);
        }
        else if (operation->type == OperationType::SEND) {
            std::cout << "[TestProtocol] Sent " << bytesTransferred << " bytes to client ID=" << client->client_id << "\n";
        }

        g_operation_manager.Release(operation);
        return 0;
    }

    int DeleteClient(IOCP_Client* client) override {
        std::cout << "[TestProtocol] Delete client ID=" << client->client_id << "\n";
        return 0;
    }
};
