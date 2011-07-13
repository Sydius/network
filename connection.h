#pragma once

#include <memory>
#include <boost/asio.hpp>
#include "invoke.h"
#include "log.h"

#define RPC(x) #x, x
#define CLIENT_RPC(x) "C$" RPC(x)
#define SERVER_RPC(x) "S$" RPC(x)

class Connection: public std::enable_shared_from_this<Connection>
{
    public:
        typedef std::shared_ptr<Connection> pointer;
        typedef invoke::Invoker<Connection::pointer> RPCInvoker;
        typedef boost::asio::io_service IOService;
        typedef std::function<void (boost::system::error_code)> DisconnectHandler;

        /**
         * Create a new connection object
         *
         * @param ioService IOService to use (not used if never connected)
         * @param invoker   RPC invoker to use with this connection
         * @return          A shared pointer to a new connection object
         */
        static pointer create(Connection::IOService & ioService, const RPCInvoker & invoker)
        {
            return pointer(new Connection(ioService, invoker));
        }

        /**
         * Create a new connection to a remote server
         *
         * @param ioService IOService to use
         * @param invoker   RPC invoker to use with this connection
         * @param hostname  Host name to connect to
         * @param port      Port to connect to
         * @return          A shared pointer to a new connection object
         */
        static pointer connect(Connection::IOService & ioService, const RPCInvoker & invoker,
                const std::string & hostname, unsigned short port)
        {
            pointer ptr = create(ioService, invoker);
            ptr->connect(hostname, port);
            return ptr;
        }

        /**
         * Retrieve the socket for the connection
         *
         * @return  Socket that belongs to this connection
         */
        boost::asio::ip::tcp::socket & socket()
        {
            return _socket;
        }

        /**
         * Begin reading on this connection
         *
         * @param disconnectHandler Function to call when this connection disconnects
         */
        void beginReading(const DisconnectHandler & disconnectHandler)
        {
            _disconnectHandler = disconnectHandler;
            _shouldCallDisconnectHandler = true;
            read();
        }

        /**
         * Execute an RPC on the other end of this connection (or immediately locally if not connected)
         *
         * @param name      Name of RPC method
         * @param function  Function definition for type-safety checking
         * @param args...   Arguments to pass to the RPC method
         */
        template<typename Function, typename... Args>
        inline void execute(std::string && name, Function function, Args && ... args)
        {
            LOG_DEBUG("Remote RPC executed: ", name);
            if (_connected) {
                remoteExecute(std::forward<std::string>(name), std::forward<Function>(function), std::forward<Args>(args)...);
            } else {
                function(std::forward<Args>(args)..., shared_from_this());
            }
        }

        /**
         * Disconnect and cleanly shut down the link
         */
        void disconnect()
        {
            _connected = false;
            if (!_lastErrorCode) {
                LOG_DEBUG("Shutting down socket");
                _socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, _lastErrorCode);
            }

            if (!_lastErrorCode) {
                LOG_DEBUG("Closing socket");
                _socket.close(_lastErrorCode);
            }

            if (_shouldCallDisconnectHandler) {
                _disconnectHandler(_lastErrorCode);
            }

            if (_lastErrorCode && _lastErrorCode != boost::asio::error::eof) {
                throw boost::system::system_error(_lastErrorCode);
            }

            LOG_DEBUG("Connection disconnected");
        }

        ~Connection()
        {
            LOG_DEBUG("Connection destroyed");
        }

    private:
        Connection(Connection::IOService & ioService, const RPCInvoker & invoker)
            : _socket(ioService)
            , _invoker(invoker)
            , _connected(false)
            , _shouldCallDisconnectHandler(false)
        {
            LOG_DEBUG("Connection created");
        }

        void connect(const std::string & hostname, unsigned short port)
        {
            using boost::asio::ip::tcp;

            tcp::resolver resolver(_socket.io_service());
            tcp::resolver::query query(hostname, "0"); // The port is set later, directly
            tcp::resolver::iterator end;
            tcp::endpoint endPoint;

            boost::system::error_code error = boost::asio::error::host_not_found;
            for (auto endpointsIter = resolver.resolve(query); error && endpointsIter != end; endpointsIter++) {
                _socket.close();
                endPoint = *endpointsIter;
                endPoint.port(port);
                LOG_INFO("Connection attempt: ", endPoint);
                _socket.connect(endPoint, error);
            }

            if (error) {
                _lastErrorCode = error;
                disconnect();
                return;
            }

            LOG_NOTICE("Connected: ", endPoint);

            read();
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

        void read()
        {
            _connected = true;
            boost::asio::async_read_until(_socket, _incoming, PACKET_END,
                std::bind(&Connection::handleRead, shared_from_this(),
                    std::placeholders::_1,
                    std::placeholders::_2));
        }

        void handleRead(const boost::system::error_code & error, size_t size)
        {
            if (error) {
                _lastErrorCode = error;
                disconnect();
                return;
            }

            std::istream is(&_incoming);
            std::string line;
            std::getline(is, line, PACKET_END);

            LOG_DEBUG("Local RPC executed: ", _invoker.extractName(line));

            _invoker.invoke(line, shared_from_this());

            if (_connected) {
                read();
            }
        }

        void handleWrite(const boost::system::error_code & error, size_t)
        {
            if (error) {
                _lastErrorCode = error;
                disconnect();
                return;
            }
        }

        boost::asio::streambuf _incoming;
        boost::asio::ip::tcp::socket _socket;
        RPCInvoker _invoker;
        bool _connected;
        DisconnectHandler _disconnectHandler;
        bool _shouldCallDisconnectHandler;
        boost::system::error_code _lastErrorCode;
        static const char PACKET_END = '\0';
};
