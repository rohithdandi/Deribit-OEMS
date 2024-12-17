#pragma once

// Standard Library Includes
#include <memory>
#include <thread>
#include <mutex>
#include <deque>
#include <string>
#include <optional>
#include <unordered_map>
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
