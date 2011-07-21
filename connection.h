#pragma once

#include <memory>
#include <boost/system/error_code.hpp>
#include <boost/functional/hash.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "invoke.h"
#include "log.h"

/**
 * Macros used to organize functions as being for both sides or just one.
 */
#define RPC(x) "rpc_" #x, x             // Both
#define CLIENT_RPC(x) "client_" RPC(x)  // Client
#define SERVER_RPC(x) "server_" RPC(x)  // Server

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++" // enable_shared_from_this doesn't need a virtual destructor
class Connection: public std::enable_shared_from_this<Connection>
{
#pragma GCC diagnostic pop
    public:
        // Pointer types
        typedef std::shared_ptr<Connection> Pointer;
        typedef std::weak_ptr<Connection> WeakPointer;

        typedef invoke::Invoker<Connection::Pointer> RPCInvoker;
        
        typedef std::unordered_map<boost::uuids::uuid, Connection::WeakPointer, boost::hash<boost::uuids::uuid>> ConnectionMap;

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
         * Execute an RPC on the other end of this connection (or immediately locally if not connected)
         *
         * @param name      Name of RPC method
         * @param function  Function definition for type-safety checking
         * @param args...   Arguments to pass to the RPC method
         */
        template<typename Function, typename... Args>
        inline void execute(std::string && name, Function function, Args && ... args)
        {
            switch (_type) {
                case Incoming:
                case Outgoing:
                    LOG_DEBUG("Remote RPC executed: ", name);
                    remoteExecute(std::forward<std::string>(name), _invoker.serialize(std::forward<std::string>(name), function, std::forward<Args>(args)...));
                    break;
                case Fake:
                    LOG_DEBUG("Fake RPC executed: ", name);
                    function(std::forward<Args>(args)..., shared_from_this());
                    break;
                default:
                    throw std::logic_error("Execute called on unknown link type");
            }
        }

        /**
         * Disconnect and cleanly shut down the link
         */
        virtual void disconnect() {}

        /**
         * Get a map of the other connections
         *
         * @return  Map containing the other connections
         */
        virtual ConnectionMap & peers() = 0;

        virtual ~Connection()
        {
            LOG_DEBUG("Connection destroyed");
        }
        
        Connection & operator=(const Connection &) = delete;
        Connection(const Connection &) = delete;

    protected:
        // Type of connection
        typedef enum { Unknown, Outgoing, Incoming, Fake } Type;

        Connection(Type type,
                   const RPCInvoker & invoker,
                   const boost::uuids::uuid & uuid = boost::uuids::nil_uuid())
            : _invoker{invoker}
            , _uuid(uuid)
            , _type{type}
        {
            LOG_DEBUG("Connection created");
        }

        RPCInvoker & invoker() { return _invoker; }

        void remoteExecute(const std::string & name, const std::string & params) {}

    private:
        RPCInvoker _invoker; // RPC methods
        boost::uuids::uuid _uuid;
        Type _type;
};
