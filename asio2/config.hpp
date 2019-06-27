/*
 * COPYRIGHT (C) 2017, zhllxt
 * 
 * author   : zhllxt
 * qq       : 37792738
 * email    : 37792738@qq.com
 * 
 */

#ifndef __ASIO2_CONFIG_HPP__
#define __ASIO2_CONFIG_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)


// if ASIO2_DUMP_EXCEPTION_LOG is defined,when important exception occured,it will print and dump the exception msg.
#define ASIO2_DUMP_EXCEPTION_LOG

// when use logger in multi module(eg:exe and dll),should #define ASIO2_LOGGER_MULTI_MODULE
#define ASIO2_LOGGER_MULTI_MODULE

// if you want to use the SSL function,eg : tcps,https, you must define ASIO2_USE_SSL
//#define ASIO2_USE_SSL

// if you want use the http and websocket module,you must define ASIO2_USE_HTTP
// note : this is a temporary solution,because i can't make boost::beast standalone
//#define ASIO2_USE_HTTP

// if you don't want to use boost in your project,you must define ASIO2_USE_ASIOSTANDALONE
// note : if ASIO2_USE_ASIOSTANDALONE is defined,the ASIO2_USE_HTTP must be not defined
//#define ASIO2_USE_ASIOSTANDALONE


#endif // !__ASIO2_CONFIG_HPP__
