#pragma once

#include <boost/asio.hpp>

#include "connection.h"

class RealConnection: public Connection
{
    public:
        typedef boost::asio::io_service IOService;
        typedef std::function<void (boost::system::error_code)> DisconnectHandler;

        /**
         * Create a new connection object
         *
         * @param invoker   RPC invoker to use with this connection
         * @param ioService IOService to use (not used if never connected)
         * @param uuid      UUID for the connection
         * @param peers     A map of peer connections
         * @return          A shared Pointer to a new connection object
         */
        static Pointer incoming(const RPCInvoker & invoker, IOService & ioService,
                const boost::uuids::uuid & uuid = boost::uuids::nil_uuid(), ConnectionMap * peers = NULL);

        /**
         * Get the socket used by this connection.
         *
         * @return  Socket used by this connection
         */
        virtual boost::asio::ip::tcp::socket & socket()
        {
            return _socket;
        }

        /**
         * Begin reading on this connection
         *
         * @param disconnectHandler Function to call when this connection disconnects
         */
        virtual void beginReading(const DisconnectHandler & disconnectHandler);

        virtual void disconnect();

        virtual ConnectionMap & peers();

        virtual ~RealConnection() {}

        RealConnection & operator=(const RealConnection &) = delete;
        RealConnection(const RealConnection &) = delete;

    protected:
        RealConnection(Type type,
                       const RPCInvoker & invoker,
                       IOService & ioService,
                       const boost::uuids::uuid & uuid = boost::uuids::nil_uuid(),
                       ConnectionMap * peers = NULL);

        void read();

        void lastErrorCode(boost::system::error_code error)
        {
            _lastErrorCode = error;
        }

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

        void write();

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
