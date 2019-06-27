/*
 * COPYRIGHT (C) 2017, zhllxt
 * 
 * author   : zhllxt
 * qq       : 37792738
 * email    : 37792738@qq.com
 *
 */

#ifndef __ASIO2_HPP__
#define __ASIO2_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)


#include <asio2/version.hpp>
#include <asio2/config.hpp>

#include <asio2/util/buffer.hpp>
#include <asio2/util/def.hpp>
#include <asio2/util/helper.hpp>
#include <asio2/util/ini.hpp>
#include <asio2/util/logger.hpp>
#include <asio2/util/pool.hpp>
#include <asio2/util/rwlock.hpp>
#include <asio2/util/spin_lock.hpp>
#include <asio2/util/thread_pool.hpp>

#include <asio2/server.hpp>
#include <asio2/client.hpp>
#include <asio2/sender.hpp>


#endif // !__ASIO2_HPP__
