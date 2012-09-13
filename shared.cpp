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
#include "shared.h"

#include <iostream>
#include <boost/lexical_cast.hpp>

void printMessage(const std::string & message, SydNet::Connection::Pointer connection)
{
    std::cout << message << std::endl;
    if (message == "Tick!") {
        connection->execute(SERVER_RPC(gotMessage));
    }
}

void sendMessage(const std::string & message, SydNet::Connection::Pointer connection)
{
    for (auto & peer: connection->peers()) {
        SydNet::Connection::Pointer{peer.second}->execute(CLIENT_RPC(printMessage), boost::lexical_cast<std::string>(connection->uuid()) + ": " + message);
    }
}

int mul(int x, SydNet::Connection::Pointer connection)
{
    return x * 5;
}

static void gotMessageResult(int result, int add)
{
    std::cout << result + add << std::endl;
}

void gotMessage(SydNet::Connection::Pointer connection)
{
    std::cout << "Got message" << std::endl;
    connection->executeCallback(CLIENT_RPC(mul), std::bind(gotMessageResult, std::placeholders::_1, 2), 3);
}

SydNet::Connection::RPCInvoker RPCMethods()
{
    SydNet::Connection::RPCInvoker invoker;
    invoker.registerFunction(CLIENT_RPC(printMessage));
    invoker.registerFunction(SERVER_RPC(sendMessage));
    invoker.registerFunction(SERVER_RPC(gotMessage));
    invoker.registerFunction(CLIENT_RPC(mul));
    return invoker;
}
