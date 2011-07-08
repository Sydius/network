#pragma once

class Connection: public std::enable_shared_from_this<Connection>
{
    public:
        typedef std::shared_ptr<Connection> pointer;

        static pointer create(boost::asio::io_service & ioService, invoke::Invoker & invoker)
        {
            return pointer(new Connection(ioService, invoker));
        }

        void connect(const std::string & hostname, const std::string & service)
        {
            boost::asio::ip::tcp::resolver resolver(_socket.io_service());
            boost::asio::ip::tcp::resolver::query query(hostname, service);
            boost::asio::ip::tcp::resolver::iterator endpointsIter = resolver.resolve(query);
            boost::asio::ip::tcp::resolver::iterator end;

            boost::system::error_code error = boost::asio::error::host_not_found;
            while (error && endpointsIter != end) {
                _socket.close();
                _socket.connect(*endpointsIter++, error);
            }
        }

        boost::asio::ip::tcp::socket & socket()
        {
            return _socket;
        }

        void beginReading()
        {
            boost::asio::async_read_until(_socket, _incoming, '\n',
                std::bind(&Connection::handleRead, shared_from_this(),
                    std::placeholders::_1,
                    std::placeholders::_2));
        }

        void write(const std::string & message)
        {
            _message = message;
            boost::asio::async_write(_socket, boost::asio::buffer(_message),
                std::bind(&Connection::handleWrite, shared_from_this(),
                    std::placeholders::_1,
                    std::placeholders::_2));
        }

    private:
        Connection(boost::asio::io_service & ioService, invoke::Invoker & invoker)
            : _socket(ioService)
            , _invoker(invoker)
        {
        }

        void handleRead(const boost::system::error_code & error, size_t size)
        {
            if (!error) {
                std::istream is(&_incoming);
                std::string line;
                std::getline(is, line);
                _invoker.invoke(line);
                beginReading();
            }
        }

        void handleWrite(const boost::system::error_code &, size_t)
        {
        }

        boost::asio::streambuf _incoming;
        boost::asio::ip::tcp::socket _socket;
        std::string _message;
        invoke::Invoker & _invoker;
};
