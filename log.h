#pragma once

#ifdef USE_PANTHEIOS
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

#define LOG_DEBUG(...) pantheios::log_DEBUG(__VA_ARGS__)
#define LOG_INFO(...) pantheios::log_INFORMATIONAL(__VA_ARGS__)
#define LOG_NOTICE(...) pantheios::log_NOTICE(__VA_ARGS__)
#define LOG_WARNING(...) pantheios::log_WARNING(__VA_ARGS__)
#define LOG_ERROR(...) pantheios::log_ERROR(__VA_ARGS__)
#define LOG_CRITICAL(...) pantheios::log_CRITICAL(__VA_ARGS__)
#define LOG_ALERT(...) pantheios::log_ALERT(__VA_ARGS__)
#define LOG_EMERGENCY(...) pantheios::log_EMERGENCY(__VA_ARGS__)

#include <pantheios/pantheios.hpp>
#include <pantheios/frontends/stock.h>

#else // USE_PANTHEIOS

#define LOG_DEBUG(...)
#define LOG_INFO(...)
#define LOG_NOTICE(...)
#define LOG_WARNING(...)
#define LOG_ERROR(...)
#define LOG_CRITICAL(...)
#define LOG_ALERT(...)
#define LOG_EMERGENCY(...)

#endif
