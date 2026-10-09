#pragma once

#include <atomic>
#include <thread>
#include <vector>

#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

class TcpReceiver
{
public:
    TcpReceiver();
    ~TcpReceiver();

    TcpReceiver(const TcpReceiver&) = delete;
    TcpReceiver& operator=(const TcpReceiver&) = delete;

    bool start(int port = 5001);
    void stop();

private:
    void worker();

    bool receiveAll(char* buffer, int size);

    SOCKET listenSocket_ = INVALID_SOCKET;
    SOCKET clientSocket_ = INVALID_SOCKET;

    int port_ = 5001;

    std::thread workerThread_;

    std::atomic<bool> running_{false};
};