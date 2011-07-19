#pragma once

#include "connection.h"

void printMessage(const std::string & message, Connection::Pointer connection);

void sendMessage(const std::string & message, Connection::Pointer connection);

Connection::RPCInvoker RPCMethods();
