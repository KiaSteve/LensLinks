#include "../tcp/tcp_receiver.h"

#include <iostream>

int main()
{
    TcpReceiver receiver;

    receiver.start(5001);

    std::cout << "LensLinks Engine Running..." << std::endl;
    std::cout << "Waiting for phone..." << std::endl;

    std::cin.get();

    receiver.stop();

    return 0;
}