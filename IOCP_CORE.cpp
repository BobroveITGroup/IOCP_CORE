#include "IOCP_CORE.h"

IOCP_OperationManager g_operation_manager; // <--- ÎÏÐÅÄÅËÅÍÈÅ

bool AsyncSend(std::shared_ptr <IOCP_Client> client, const uint8_t* data, size_t size, DWORD flags) {
    if (!client || size == 0 || !data) return false;

    SOCKET sock = INVALID_SOCKET;
    sockaddr_in* addr = nullptr;

    if (client->is_udp_initialized) {
        if (client->udp_socket == INVALID_SOCKET || !client->is_udp_initialized) {
            return false; // UDP ñîêåò èëè àäðåñ íå ãîòîâû
        }
        sock = client->udp_socket;
        addr = &client->udp_address;
    }
    else {
        if (client->tcp_socket == INVALID_SOCKET) {
            return false;
        }
        sock = client->tcp_socket;
    }

    IOCP_Operation* op = g_operation_manager.Acquire(OperationType::SEND, size, client);
    std::copy(data, data + size, op->buffer.begin());
    op->wsa_buf.len = static_cast<ULONG>(size);

    DWORD bytesSent = 0;

    int result = SOCKET_ERROR;

    if (addr) {
        // UDP
        result = WSASendTo(sock,
            &op->wsa_buf,
            1,
            &bytesSent,
            flags,
            reinterpret_cast<SOCKADDR*>(addr),
            sizeof(sockaddr_in),
            &op->overlapped,
            nullptr);
    }
    else {
        // TCP
        result = WSASend(sock,
            &op->wsa_buf,
            1,
            &bytesSent,
            flags,
            &op->overlapped,
            nullptr);
    }

    if (result == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err != WSA_IO_PENDING) {
            g_operation_manager.Release(op);
            return false;
        }
    }
    return true;
}
bool AsyncRecv(std::shared_ptr <IOCP_Client> client, size_t buffer_size, DWORD flags) {
    if (!client) return false;

    SOCKET sock = INVALID_SOCKET;
    sockaddr_in* addr = nullptr;
    int addr_len = sizeof(sockaddr_in);

    if (client->is_udp_initialized) {
        if (client->udp_socket == INVALID_SOCKET) {
            return false;
        }
        sock = client->udp_socket;
        addr = &client->udp_address;
    }
    else {
        if (client->tcp_socket == INVALID_SOCKET) {
            return false;
        }
        sock = client->tcp_socket;
    }

    IOCP_Operation* op = g_operation_manager.Acquire(OperationType::RECV, buffer_size, client);

    DWORD bytesReceived = 0;

    int result = SOCKET_ERROR;

    if (addr) {
        // UDP
        result = WSARecvFrom(sock,
            &op->wsa_buf,
            1,
            &bytesReceived,
            &flags,
            reinterpret_cast<SOCKADDR*>(addr),
            &addr_len,
            &op->overlapped,
            nullptr);
    }
    else {
        // TCP
        result = WSARecv(sock,
            &op->wsa_buf,
            1,
            &bytesReceived,
            &flags,
            &op->overlapped,
            nullptr);
    }

    if (result == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err != WSA_IO_PENDING) {
            g_operation_manager.Release(op);
            return false;
        }
    }
    return true;
}