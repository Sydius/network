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
                const boost::uuids::uuid & uuid = boost::uuids::nil_uuid(), ConnectionMap * connections = NULL);

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
                const std::string & hostname, unsigned short port);

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
        void beginReading(const DisconnectHandler & disconnectHandler);

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
        void disconnect();

        /**
         * Get a map of the other connections
         *
         * @return  Map containing the other connections
         */
        ConnectionMap & peers();

        ~Connection()
        {
            LOG_DEBUG("Connection destroyed");
        }

    private:
        Connection(Connection::IOService & ioService, const RPCInvoker & invoker, const boost::uuids::uuid & uuid, ConnectionMap * connections);

        void connect(const std::string & hostname, unsigned short port);

        template<typename Function, typename... Args>
        void remoteExecute(std::string && name, Function function, Args && ... args)
        {
            std::string toSend{_invoker.serialize(std::forward<std::string>(name), function, std::forward<Args>(args)...)};

            boost::asio::async_write(_socket, boost::asio::buffer(toSend + PACKET_END),
                std::bind(&Connection::handleWrite, shared_from_this(),
                    std::placeholders::_1,
                    std::placeholders::_2));
        }

        void read();

        void handleRead(const boost::system::error_code & error, size_t size);

        void handleWrite(const boost::system::error_code & error, size_t);

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
