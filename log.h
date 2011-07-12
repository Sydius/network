#pragma once

#include <boost/lexical_cast.hpp>
#include <stlsoft/string/shim_string.hpp>

namespace stlsoft
{
    template <class T>
    char const * c_str_data_a(T const & t)
    {
        static std::string output;
        output = boost::lexical_cast<std::string>(t);
        return output.c_str();
    }

    template <class T>
    size_t c_str_len_a(T const & t)
    {
        static std::string output;
        output = boost::lexical_cast<std::string>(t);
        return output.length();
    }
}

#define LOG_DEBUG pantheios::log_DEBUG
#define LOG_INFO pantheios::log_INFORMATIONAL
#define LOG_NOTICE pantheios::log_NOTICE
#define LOG_WARNING pantheios::log_WARNING
#define LOG_ERROR pantheios::log_ERROR
#define LOG_CRITICAL pantheios::log_CRITICAL
#define LOG_ALERT pantheios::log_ALERT
#define LOG_EMERGENCY pantheios::log_EMERGENCY

#include <pantheios/pantheios.hpp>
#include <pantheios/frontends/stock.h>
