#pragma once

#include <memory>
#include <boost/asio.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "invoke.h"
#include "log.h"

#define RPC(x) "rpc_" #x, x
#define CLIENT_RPC(x) "client_" RPC(x)
#define SERVER_RPC(x) "server_" RPC(x)

class Connection: public std::enable_shared_from_this<Connection>
{
    public:
        typedef std::shared_ptr<Connection> pointer;
        typedef invoke::Invoker<Connection::pointer> RPCInvoker;
        typedef boost::asio::io_service IOService;
        typedef std::function<void (boost::system::error_code)> DisconnectHandler;
        typedef std::unordered_map<boost::uuids::uuid, Connection::pointer, boost::hash<boost::uuids::uuid>> ConnectionMap;

        /**
         * Create a new connection object
         *
         * @param ioService IOService to use (not used if never connected)
         * @param invoker   RPC invoker to use with this connection
         * @param uuid      UUID for the connection
         * @return          A shared pointer to a new connection object
         */
        static pointer create(Connection::IOService & ioService, const RPCInvoker & invoker,
                const boost::uuids::uuid & uuid = boost::uuids::nil_uuid(), ConnectionMap * connections = NULL)
        {
            return pointer(new Connection(ioService, invoker, uuid, connections));
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
         * Get the UUID of the connection.
         *
         * @return  UUID of the connection
         */
        boost::uuids::uuid uuid() const
        {
            return _uuid;
        }

        /**
         * Set the UUID of the connection
         *
         * @param uuid  UUID to set the connect to
         */
        void uuid(const boost::uuids::uuid & uuid)
        {
            _uuid = uuid;
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
                LOG_DEBUG("Disconnect handler being called");
                _disconnectHandler(_lastErrorCode);
            }

            if (_lastErrorCode && _lastErrorCode != boost::asio::error::eof) {
                throw boost::system::system_error(_lastErrorCode);
            }

            LOG_DEBUG("Connection disconnected");
        }

        /**
         * Get a map of the other connections
         *
         * @return  Map containing the other connections
         */
        ConnectionMap & peers()
        {
            if (!_connections) {
                throw std::logic_error("An attempt to walk connections when there are none was made");
            }
            return *_connections;
        }

        ~Connection()
        {
            LOG_DEBUG("Connection destroyed");
        }

    private:
        Connection(Connection::IOService & ioService, const RPCInvoker & invoker, const boost::uuids::uuid & uuid, ConnectionMap * connections)
            : _socket(ioService)
            , _invoker(invoker)
            , _connected(false)
            , _shouldCallDisconnectHandler(false)
            , _uuid(uuid)
            , _connections(connections)
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
        boost::uuids::uuid _uuid;
        ConnectionMap * _connections;

        static const char PACKET_END = '\0';
};
