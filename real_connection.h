#pragma once

#include "connection.h"

#include <boost/asio.hpp>

class RealConnection: public Connection
{
    public:
        typedef boost::asio::io_service IOService;

        /**
         * Create a new connection object
         *
         * @param ioService IOService to use (not used if never connected)
         * @param invoker   RPC invoker to use with this connection
         * @param uuid      UUID for the connection
         * @param peers     A map of peer connections
         * @return          A shared Pointer to a new connection object
         */
        static Pointer incoming(IOService & ioService, const RPCInvoker & invoker,
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
        static Pointer outgoing(IOService & ioService, const RPCInvoker & invoker,
                const std::string & hostname, unsigned short port);

        virtual boost::asio::ip::tcp::socket & socket()
        {
            return _socket;
        }

        virtual void beginReading(const DisconnectHandler & disconnectHandler);

        virtual void disconnect();

        virtual ConnectionMap & peers();

        virtual ~RealConnection()
        {
        }

        RealConnection & operator=(const RealConnection &) = delete;
        RealConnection(const RealConnection &) = delete;

    protected:
        RealConnection(Type type,
                       IOService & ioService,
                       const RPCInvoker & invoker,
                       const boost::uuids::uuid & uuid = boost::uuids::nil_uuid(),
                       ConnectionMap * peers = NULL);

    private:
        void remoteExecute(const std::string & name, const std::string & params)
        {
            std::ostream outgoingStream{&_outgoing};
            outgoingStream << name << PACKET_END;
            outgoingStream << params;
            outgoingStream << PACKET_END;

            if (!_writing) {
                _writing = true;
                write();
            }
        }

        std::shared_ptr<RealConnection> getDerivedPointer()
        {
            return std::static_pointer_cast<RealConnection>(shared_from_this());
        }

        void connect(const std::string & hostname, unsigned short port);

        void write();

        void read();

        void handleRead(const boost::system::error_code & error, size_t size);

        void handleWrite(const boost::system::error_code & error, size_t);

        boost::asio::streambuf _incoming; // For incoming data; must stay valid while reading
        boost::asio::streambuf _outgoing; // For outgoing data; must stay valid while writing
        bool _writing; // True if it's already sending data
        boost::asio::ip::tcp::socket _socket;
        bool _connected;
        DisconnectHandler _disconnectHandler;
        boost::system::error_code _lastErrorCode;
        ConnectionMap * _peers; // Peer connections

        static const char PACKET_END = '\0';
};
