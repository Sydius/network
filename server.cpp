#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include "shared.h"
#include "invoke.h"

class Connection: public std::enable_shared_from_this<Connection>
{
    public:
        typedef std::shared_ptr<Connection> pointer;

        static pointer create(boost::asio::io_service & ioService, invoke::Invoker & invoker)
        {
            return pointer(new Connection(ioService, invoker));
        }

        boost::asio::ip::tcp::socket & socket()
        {
            return _socket;
        }

        void start()
        {
            boost::asio::async_read_until(_socket, _incoming, '\n',
                std::bind(&Connection::handleRead, shared_from_this(),
                    std::placeholders::_1,
                    std::placeholders::_2));
        }

    private:
        Connection(boost::asio::io_service & ioService, invoke::Invoker & invoker)
            : _socket(ioService)
            , _invoker(invoker)
        {
        }

        void handleRead(const boost::system::error_code &, size_t)
        {
            std::istream is(&_incoming);
            std::string line;
            std::getline(is, line);
            _invoker.invoke(line);
        }

        void handleWrite(const boost::system::error_code &, size_t)
        {
        }

        boost::asio::streambuf _incoming;
        boost::asio::ip::tcp::socket _socket;
        std::string _message;
        invoke::Invoker & _invoker;
};

class Server
{
    public:
        Server(boost::asio::io_service & ioService)
            : _acceptor(ioService, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 2000))
        {
            _invoker.registerFunction("foo", foo);
            startAccept();
        }

    private:
        void startAccept()
        {
            Connection::pointer newConnection = Connection::create(_acceptor.io_service(), _invoker);
            _acceptor.async_accept(newConnection->socket(),
                std::bind(&Server::handleAccept, this, newConnection, std::placeholders::_1));
        }

        void handleAccept(Connection::pointer newConnection, const boost::system::error_code & error)
        {
            if (!error) {
                newConnection->start();
                startAccept();
            }
        }

        boost::asio::ip::tcp::acceptor _acceptor;
        invoke::Invoker _invoker;
};

int main(int argc, char * argv[])
{
    std::cout << invoke::serialize("foo", foo, 5) << std::endl;
    try {
        boost::asio::io_service ioService;
        Server server(ioService);
        ioService.run();
    } catch (std::exception & e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
