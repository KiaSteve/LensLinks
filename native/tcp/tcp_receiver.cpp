#include "tcp_receiver.h"

#include "../decoder/jpeg_decoder.h"

#include "../shared/frame_queue.h"

#include <iostream>
#include <vector>
#include <ws2tcpip.h>

TcpReceiver::TcpReceiver()
{
}

TcpReceiver::~TcpReceiver()
{
    stop();
}

bool TcpReceiver::start(int port)
{
    if (running_)
        return true;

    port_ = port;

    running_ = true;

    workerThread_ = std::thread(&TcpReceiver::worker, this);

    return true;
}

void TcpReceiver::stop()
{
    running_ = false;

    if (clientSocket_ != INVALID_SOCKET)
    {
        closesocket(clientSocket_);
        clientSocket_ = INVALID_SOCKET;
    }

    if (listenSocket_ != INVALID_SOCKET)
    {
        closesocket(listenSocket_);
        listenSocket_ = INVALID_SOCKET;
    }

    WSACleanup();

    if (workerThread_.joinable())
        workerThread_.join();
}

bool TcpReceiver::receiveAll(char* buffer, int size)
{
    int received = 0;

    while (received < size)
    {
        int r = recv(
            clientSocket_,
            buffer + received,
            size - received,
            0);

        if (r <= 0)
            return false;

        received += r;
    }

    return true;
}

void TcpReceiver::worker()
{
    WSADATA wsa;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        std::cout << "WSAStartup failed\n";
        return;
    }

    listenSocket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (listenSocket_ == INVALID_SOCKET)
    {
        std::cout << "Socket creation failed\n";
        return;
    }

    sockaddr_in server{};

    server.sin_family = AF_INET;
    server.sin_port = htons(port_);
    server.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenSocket_,
             reinterpret_cast<sockaddr*>(&server),
             sizeof(server)) == SOCKET_ERROR)
    {
        std::cout << "Bind failed\n";
        return;
    }

    listen(listenSocket_, 1);

    JpegDecoder decoder;

    std::cout << "Waiting for phone on port "
              << port_
              << std::endl;

    while (running_)
    {
        clientSocket_ = accept(
            listenSocket_,
            nullptr,
            nullptr);

        if (clientSocket_ == INVALID_SOCKET)
            continue;

        std::cout << "Phone Connected!" << std::endl;

                while (running_)
        {
            uint32_t length = 0;

            if (!receiveAll(reinterpret_cast<char*>(&length), 4))
                break;

            length = ntohl(length);

            if (length == 0 || length > 20 * 1024 * 1024)
            {
                std::cout << "Invalid frame size" << std::endl;
                break;
            }

            std::vector<uint8_t> jpeg(length);

            if (!receiveAll(reinterpret_cast<char*>(jpeg.data()), length))
                break;

            Frame decodedFrame;

            if (decoder.decode(jpeg, decodedFrame))
            {
                std::cout
                    << "Decoded frame: "
                    << decodedFrame.width
                    << "x"
                    << decodedFrame.height
                    << std::endl;

                FrameQueue::instance().setFrame(std::move(decodedFrame));
            }
            else
            {
                std::cout << "JPEG decode failed." << std::endl;
            }
        }   // <-- CLOSES the inner while loop

        std::cout << "Phone disconnected." << std::endl;

        closesocket(clientSocket_);
        clientSocket_ = INVALID_SOCKET;

    }   // <-- CLOSES the outer while loop

    if (listenSocket_ != INVALID_SOCKET)
    {
        closesocket(listenSocket_);
        listenSocket_ = INVALID_SOCKET;
    }

    WSACleanup();
}