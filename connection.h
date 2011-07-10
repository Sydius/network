#pragma once

class Connection: public std::enable_shared_from_this<Connection>
{
    public:
        typedef std::shared_ptr<Connection> pointer;
        typedef invoke::Invoker<> RPCInvoker;

        static pointer create(boost::asio::io_service & ioService, const RPCInvoker & invoker)
        {
            return pointer(new Connection(ioService, invoker));
        }

        static pointer connect(boost::asio::io_service & ioService, const RPCInvoker & invoker,
                const std::string & hostname, const std::string & service)
        {
            pointer ptr = create(ioService, invoker);
            ptr->connect(hostname, service);
            return ptr;
        }

        void connect(const std::string & hostname, const std::string & service)
        {
            using boost::asio::ip::tcp;

            tcp::resolver resolver(_socket.io_service());
            tcp::resolver::query query(hostname, service);
            tcp::resolver::iterator endpointsIter = resolver.resolve(query);
            tcp::resolver::iterator end;

            boost::system::error_code error = boost::asio::error::host_not_found;
            while (error && endpointsIter != end) {
                _socket.close();
                _socket.connect(*endpointsIter++, error);
            }

            if (error) {
                throw boost::system::system_error(error);
            }
        }

        boost::asio::ip::tcp::socket & socket()
        {
            return _socket;
        }

        void beginReading()
        {
            boost::asio::async_read_until(_socket, _incoming, '\0',
                std::bind(&Connection::handleRead, shared_from_this(),
                    std::placeholders::_1,
                    std::placeholders::_2));
        }

        void write(const std::string & message)
        {
            boost::asio::async_write(_socket, boost::asio::buffer(message + '\0'),
                std::bind(&Connection::handleWrite, shared_from_this(),
                    std::placeholders::_1,
                    std::placeholders::_2));
        }

    private:
        Connection(boost::asio::io_service & ioService, const RPCInvoker & invoker)
            : _socket(ioService)
            , _invoker(invoker)
        {
        }

        void handleRead(const boost::system::error_code & error, size_t size)
        {
            if (error) {
                if (error != boost::asio::error::eof) {
                    throw boost::system::system_error(error);
                }
                return;
            }

            std::istream is(&_incoming);
            std::string line;
            std::getline(is, line, '\0');
            _invoker.invoke(line);
            beginReading();
        }

        void handleWrite(const boost::system::error_code & error, size_t)
        {
            if (error && error != boost::asio::error::eof) {
                throw boost::system::system_error(error);
            }
        }

        boost::asio::streambuf _incoming;
        boost::asio::ip::tcp::socket _socket;
        RPCInvoker _invoker;
};
