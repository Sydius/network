#pragma once

#include "connection.h"

void printMessage(const std::string & message, SydNet::Connection::Pointer connection);

void sendMessage(const std::string & message, SydNet::Connection::Pointer connection);

SydNet::Connection::RPCInvoker RPCMethods();
