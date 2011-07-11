#pragma once

#include <memory>
#include <boost/asio.hpp>
#include "invoke.h"

#define RPC(x) #x, x
#define CLIENT_RPC(x) "C$" RPC(x)
#define SERVER_RPC(x) "S$" RPC(x)

class Connection: public std::enable_shared_from_this<Connection>
{
    public:
        typedef std::shared_ptr<Connection> pointer;
        typedef invoke::Invoker<Connection::pointer> RPCInvoker;
        typedef boost::asio::io_service IOService;

        static pointer create(Connection::IOService & ioService, const RPCInvoker & invoker)
        {
            return pointer(new Connection(ioService, invoker));
        }

        static pointer connect(Connection::IOService & ioService, const RPCInvoker & invoker,
                const std::string & hostname, unsigned short port)
        {
            pointer ptr = create(ioService, invoker);
            ptr->connect(hostname, port);
            return ptr;
        }

        void connect(const std::string & hostname, unsigned short port)
        {
            using boost::asio::ip::tcp;

            tcp::resolver resolver(_socket.io_service());
            tcp::resolver::query query(hostname, "0"); // The port is set later, directly
            tcp::resolver::iterator end;

            boost::system::error_code error = boost::asio::error::host_not_found;
            for (auto endpointsIter = resolver.resolve(query); error && endpointsIter != end; endpointsIter++) {
                _socket.close();
                tcp::endpoint endPoint = *endpointsIter;
                endPoint.port(port);
                _socket.connect(endPoint, error);
            }

            if (error) {
                throw boost::system::system_error(error);
            }

            beginReading();
        }

        boost::asio::ip::tcp::socket & socket()
        {
            return _socket;
        }

        void beginReading()
        {
            _noConnection = false;
            boost::asio::async_read_until(_socket, _incoming, PACKET_END,
                std::bind(&Connection::handleRead, shared_from_this(),
                    std::placeholders::_1,
                    std::placeholders::_2));
        }

        template<typename Function, typename... Args>
        inline void execute(std::string && name, Function function, Args && ... args)
        {
            if (_noConnection) {
                function(std::forward<Args>(args)..., shared_from_this());
            } else {
                remoteExecute(std::forward<std::string>(name), std::forward<Function>(function), std::forward<Args>(args)...);
            }
        }

    private:
        Connection(Connection::IOService & ioService, const RPCInvoker & invoker)
            : _socket(ioService)
            , _invoker(invoker)
            , _noConnection(true)
        {
        }

        template<typename Function, typename... Args>
        void remoteExecute(std::string && name, Function function, Args && ... args)
        {
            std::string toSend{_invoker.serialize(std::forward<std::string>(name), function, std::forward<Args>(args)...)};

            boost::asio::async_write(_socket, boost::asio::buffer(toSend + PACKET_END),
                std::bind(&Connection::handleWrite, shared_from_this(),
                    std::placeholders::_1,
                    std::placeholders::_2));
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
            std::getline(is, line, PACKET_END);
            _invoker.invoke(line, shared_from_this());
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
        bool _noConnection;
        static const char PACKET_END = '\0';
};
