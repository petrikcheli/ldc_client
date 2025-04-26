#include <boost/asio.hpp>
#include "server.h"

int main() {
    boost::asio::io_context io_context;
    auto work_guard = boost::asio::make_work_guard(io_context);
    Server server(io_context);



    std::thread t1([&]{io_context.run();});
    std::thread t2([&]{io_context.run();});
    std::thread t3([&]{io_context.run();});
    std::thread t4([&]{io_context.run();});
    t1.detach();
    t2.detach();
    t3.detach();
    t4.detach();
    io_context.run();

    return 0;
}
