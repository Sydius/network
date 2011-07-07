#pragma once

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
