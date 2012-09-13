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

#ifdef USE_PANTHEIOS
#include <boost/lexical_cast.hpp>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
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
#pragma GCC diagnostic pop

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
