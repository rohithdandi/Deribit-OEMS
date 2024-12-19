#pragma once

// Standard Library Includes
#include <memory>
#include <thread>
#include <mutex>
#include <string>
#include <optional>
#include <vector>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <fstream>

// Boost Library Includes
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

// System Libraries
#include <cstdlib>
#include <cstdio>
#include <functional>
#include <queue>
#include <deque>
#include <unordered_map>
#include <unordered_set>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
typedef boost::asio::io_context::executor_type executor_type;
typedef boost::asio::strand<executor_type> strand;
