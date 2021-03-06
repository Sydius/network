/*
Copyright 2011 Christopher Allen Ogden. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are
permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this list of
      conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice, this list
      of conditions and the following disclaimer in the documentation and/or other materials
      provided with the distribution.

THIS SOFTWARE IS PROVIDED BY CHRISTOPHER ALLEN OGDEN ``AS IS'' AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL CHRISTOPHER ALLEN OGDEN OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those of the
authors and should not be interpreted as representing official policies, either expressed
or implied, of Christopher Allen Ogden.
*/
#pragma once

#include <memory>
#include <boost/system/error_code.hpp>
#include <boost/functional/hash.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

#include "invoke.h"
#include "log.h"

/**
 * Macros used to organize functions as being for both sides or just one.
 */
#define RPC(x) "rpc_" #x, x             // Both
#define CLIENT_RPC(x) "client_" RPC(x)  // Client
#define SERVER_RPC(x) "server_" RPC(x)  // Server

namespace SydNet {

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++" // enable_shared_from_this doesn't need a virtual destructor
class Connection: public std::enable_shared_from_this<Connection>
{
#pragma GCC diagnostic pop
    public:
        // Pointer types
        typedef std::shared_ptr<Connection> Pointer;
        typedef std::weak_ptr<Connection> WeakPointer;

        // Stores RPC methods
        typedef invoke::Invoker<boost::archive::binary_iarchive, boost::archive::binary_oarchive, Pointer> RPCInvoker;
        
        // Maps connections to UUID
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
        inline void execute(std::string && name, Function function, Args && ... args);

        template<typename Function, typename Callback, typename... Args>
        inline void executeCallback(std::string && name, Function function, Callback callback, Args && ... args);

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
        
        // Can't copy this class
        Connection & operator=(const Connection &) = delete;
        Connection(const Connection &) = delete;

    protected:
        // Type of connection
        typedef enum { Unknown, Outgoing, Incoming, Fake } Type;

        // Protected to force the use of the factory methods in children classes
        Connection(Type type,
                   const RPCInvoker & invoker,
                   const boost::uuids::uuid & uuid = boost::uuids::nil_uuid())
            : _invoker{invoker}
            , _uuid(uuid)
            , _type{type}
        {
            LOG_DEBUG("Connection created");
        }

        // For child class access to the invoker
        RPCInvoker & invoker() { return _invoker; }

        // This needs to exist here for the template method execute to be able to pass on calls
        typedef uint16_t RequestID;
        static const RequestID REQUEST_ID_RECEIVED_BIT = 0x8000;
        virtual void remoteExecute(const std::string & name, const std::string & params, RequestID=0) {}
        typedef std::function<void(std::istream &)> RemoteExecuteCallback;
        virtual void remoteExecute(const std::string & name, const std::string & params, RemoteExecuteCallback callback) {}

    private:
        RPCInvoker _invoker; // RPC methods
        boost::uuids::uuid _uuid;
        Type _type;
};

template<typename Function, typename... Args>
void Connection::execute(std::string && name, Function function, Args && ... args)
{
    if (_type != Fake) {
        std::stringstream serialized;
        _invoker.serialize(std::forward<std::string>(name), function, serialized, std::forward<Args>(args)...);
        remoteExecute(std::forward<std::string>(name), serialized.str());
    } else {
        function(std::forward<Args>(args)..., shared_from_this());
    }
}

template<typename Function, typename Callback, typename... Args>
void Connection::executeCallback(std::string && name, Function function, Callback callback, Args && ... args)
{
    if (_type != Fake) {
        std::stringstream serialized;
        _invoker.serialize(std::forward<std::string>(name), function, serialized, std::forward<Args>(args)...);
        remoteExecute(std::forward<std::string>(name), serialized.str(),
            [this, name, function, callback](std::istream & resultStream) {
                callback(_invoker.deserialize(name, function, resultStream));
            }
        );
    } else {
        callback(function(std::forward<Args>(args)..., shared_from_this()));
    }
}

}
