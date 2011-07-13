#pragma once

#include "connection.h"

void printMessage(const std::string & message, Connection::pointer connection);

void sendMessage(const std::string & message, Connection::pointer connection);

Connection::RPCInvoker RPCMethods();
