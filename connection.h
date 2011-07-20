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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++" // enable_shared_from_this doesn't need a virtual destructor
class Connection: public std::enable_shared_from_this<Connection>
{
#pragma GCC diagnostic pop
    public:
        typedef std::shared_ptr<Connection> Pointer;
        typedef std::weak_ptr<Connection> WeakPointer;
        typedef invoke::Invoker<Connection::Pointer> RPCInvoker;
        typedef boost::asio::io_service IOService;
        typedef std::function<void (boost::system::error_code)> DisconnectHandler;
        typedef std::unordered_map<boost::uuids::uuid, Connection::WeakPointer, boost::hash<boost::uuids::uuid>> ConnectionMap;

        /**
         * Create a new connection object
         *
         * @param ioService IOService to use (not used if never connected)
         * @param invoker   RPC invoker to use with this connection
         * @param uuid      UUID for the connection
         * @param peers     A map of peer connections
         * @return          A shared Pointer to a new connection object
         */
        static Pointer incoming(Connection::IOService & ioService, const RPCInvoker & invoker,
                const boost::uuids::uuid & uuid = boost::uuids::nil_uuid(), ConnectionMap * peers = NULL);

        /**
         * Create a new connection to a remote server
         *
         * @param ioService IOService to use
         * @param invoker   RPC invoker to use with this connection
         * @param hostname  Host name to connect to
         * @param port      Port to connect to
         * @return          A shared Pointer to a new connection object
         */
        static Pointer outgoing(Connection::IOService & ioService, const RPCInvoker & invoker,
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
        virtual void beginReading(const DisconnectHandler & disconnectHandler);

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
            if (_connected) {
                LOG_DEBUG("Remote RPC executed: ", name);
                remoteExecute(std::forward<std::string>(name), std::forward<Function>(function), std::forward<Args>(args)...);
            } else {
                LOG_DEBUG("Fake RPC executed: ", name);
                function(std::forward<Args>(args)..., shared_from_this());
            }
        }

        /**
         * Disconnect and cleanly shut down the link
         */
        virtual void disconnect();

        /**
         * Get a map of the other connections
         *
         * @return  Map containing the other connections
         */
        virtual ConnectionMap & peers();

        virtual ~Connection()
        {
            LOG_DEBUG("Connection destroyed");
        }
        
        Connection & operator=(const Connection &) = delete;
        Connection(const Connection &) = delete;

    protected:
        Connection(Connection::IOService & ioService,
                   const RPCInvoker & invoker,
                   const boost::uuids::uuid & uuid = boost::uuids::nil_uuid(),
                   ConnectionMap * peers = NULL);

    private:
        void connect(const std::string & hostname, unsigned short port);

        template<typename Function, typename... Args>
        void remoteExecute(std::string && name, Function function, Args && ... args)
        {
            std::ostream outgoingStream{&_outgoing};
            outgoingStream << name << PACKET_END;
            outgoingStream << _invoker.serialize(std::forward<std::string>(name), function, std::forward<Args>(args)...);
            outgoingStream << PACKET_END;

            if (!_writing) {
                _writing = true;
                write();
            }
        }

        void write();

        void read();

        void handleRead(const boost::system::error_code & error, size_t size);

        void handleWrite(const boost::system::error_code & error, size_t);

        boost::asio::streambuf _incoming; // For incoming data; must stay valid while reading
        boost::asio::streambuf _outgoing; // For outgoing data; must stay valid while writing
        bool _writing; // True if it's already sending data
        boost::asio::ip::tcp::socket _socket;
        RPCInvoker _invoker; // RPC methods
        bool _connected;
        DisconnectHandler _disconnectHandler;
        boost::system::error_code _lastErrorCode;
        boost::uuids::uuid _uuid;
        ConnectionMap * _peers; // Peer connections

        static const char PACKET_END = '\0';
};
