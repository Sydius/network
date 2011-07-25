#pragma once

#include <boost/asio.hpp>

#include "io_service.h"
#include "connection.h"

namespace SydNet {

class RealConnection: public Connection
{
    public:
        /**
         * Get the socket used by this connection.
         *
         * @return  Socket used by this connection
         */
        virtual boost::asio::ip::tcp::socket & socket()
        {
            return _socket;
        }

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

        void read(size_t size=0);

        boost::system::error_code lastErrorCode() const
        {
            return _lastErrorCode;
        }

        void lastErrorCode(boost::system::error_code error)
        {
            _lastErrorCode = error;
        }

    private:
        void remoteExecute(const std::string & name, const std::string & params);

        std::shared_ptr<RealConnection> getDerivedPointer()
        {
            return std::static_pointer_cast<RealConnection>(shared_from_this());
        }

        void write();

        void handleReadCommandHeader(const boost::system::error_code & error, size_t size);

        void handleReadCommand(const boost::system::error_code & error, size_t size, uint16_t commandSize);

        void handleWrite(const boost::system::error_code & error, size_t);

        boost::asio::streambuf _incoming; // For incoming data; must stay valid while reading
        boost::asio::streambuf _outgoing; // For outgoing data; must stay valid while writing
        bool _writing; // True if it's already sending data
        boost::asio::ip::tcp::socket _socket;
        bool _connected;
        boost::system::error_code _lastErrorCode;
        ConnectionMap * _peers; // Peer connections

        static const char PACKET_END = '\0';
};

}
