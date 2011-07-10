#pragma once

#include "connection.h"

void ping(int x, Connection::pointer connection);

void pong(int x, const std::string & message, Connection::pointer connection);

Connection::RPCInvoker RPCMethods();
