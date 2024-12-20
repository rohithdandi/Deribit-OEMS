# Order Execution and Management System for Deribit Test API

## Overview

This project is a high-performance Command-Line Interface (CLI) application that facilitates order execution and management on the Deribit Test WebSocket API. Designed for efficient trading, the system employs modern C++ standards and powerful libraries to ensure robustness, scalability, and ease of use.

This project implements a robust, asynchronous, thread-safe WebSocket client using Boost.Beast and Boost.Asio. The implementation ensures secure and efficient communication with the Deribit API, managing multiple operations such as authentication, message transmission, and subscription data processing concurrently. The use of strands guarantees thread safety, while thread-safe queues (inbox_ and feedQueue_) handle responses and real-time subscription data without race conditions, enabling reliable data flow across different parts of the application.

![alt text](image.png)

!!!WORK IN PROGRESS!!!

## Further Improvements

- Enhanced User Interface (UI)
  Utilizing the thread-safe inbox_ for responses and feedQueue_ for subscription data, real-time information can be displayed in a single screen with multiple dynamic windows. This enhancement will provide users with:

 - Real-time Order Book View: Instant updates on market orders.
 - Trade Execution Feedback: Clear, interactive feedback for submitted trades.
 - Subscriptions Dashboard: A centralized window for all active subscriptions

- adding validation and support for more order types.


## Getting Started
To explore the available options, type:

```bash
\Deribit > $--help
```

### Available Commands

### General Commands
1. Help
  Produce help message.
```bash
--help
```

2. Connect
  Connect to the Deribit WebSocket API and authenticate.
```bash
--connect
```

### Order Management
3. Auth
  Authenticate with the Deribit Test WebSocket API.
```bash
--auth
```

4. Place
  Place a new order. Below are the required and optional parameters.

  - **`--direction <buy|sell>`** *(Required)*
  - **`--instrument_name <string>`** *(Required)*
  - **`--type <limit|stop_limit|market|stop_market>`** *(Optional)* Default: `limit`.
  - **`--amount <positive double>`** *(At least one required)*
  - **`--contracts <positive double>`** *(At least one required)*. If both `--amount` and `--contracts` are provided, their values must match.
  - **`--price <positive double>`** *(Required for `limit` and `stop_limit`)*.
  - **`--trigger_price <positive double>`** *(Required for `stop_limit` and `stop_market`)*. Tick size: 0.1.
  - **`--trigger <index_price|mark_price|last_price>`** *(Required for `stop_limit`)*.
  - **`--label <string>`** *(Optional)* Max length: 64 characters.

  Additional options:
  - **`--post_only <bool>`**
  - **`--reject_post_only <bool>`**
  - **`--mmp <bool>`**
  - **`--valid_until <int>`**
  - **`--trigger_offset <positive double>`**

```bash
--place --direction <buy|sell> --instrument_name <string> --amount <positive double> --price <positive double> --trigger_price <positive double>
```

5. Cancel
  Cancel an order. Required parameters:
  - **`--order_id <string>`**
```bash
--cancel --order_id <string>
```

### Market Data Commands

6. Get_order_book
  Get the order book for a specific instrument. Below are the required parameters.
  Required parameters:
  - **`--instrument_name <string>`**
  - **`--depth <1|5|10|20|50|100|1000|10000>`**
```bash
--get_order_book --instrument_name <string> --depth <1|5|10|20|50|100|1000|10000>
```

7. Subscribe
  Subscribe to one or more channels and instruments. Below are the required parameters.
  Required parameters:
  - **`--instrument_name <string>`**
  - **`--channel <string>`**(pairs)
```bash
--subscribe --channel <string> --instrument_name <string>
```

8. Unsubscribe
  Required parameters:
    - **`--channel <string>`**
    - **`--instrument_name <string>`**(pairs)
```bash
--unsubscribe --channel <string> --instrument_name <string>
```

9. Unsubscribe_all
  Unsubscribe from all the channels and instruments currently subscribed to.
```bash
--unsubscribe_all
```

### Program Control

10. Exit
  Exit the program.
```bash
--exit
```

## Example Usage
```bash
./websocket_tool --connect
./websocket_tool --place --direction buy --instrument_name BTC-USD --amount 1 --price 40000 --trigger_price 41000
./websocket_tool --get_order_book --instrument_name BTC-USD --depth 10
./websocket_tool --subscribe --channel ticker --instrument_name BTC-USD
./websocket_tool --unsubscribe --channel ticker --instrument_name BTC-USD
```

---


## Features

- WebSocket Connectivity: Connect to Deribit WebSocket API and  authenticate.

- Order Management: Place, modify, and cancel orders with comprehensive parameter support.

- Market Data Retrieval: Fetch order books and subscribe to live updates.

- Subscriptions Management: Subscribe/unsubscribe to/from channels dynamically.

- Multi-threaded I/O: Efficient handling of I/O operations using Boost Asio.

## Project Structure

```sh
.
|-- Header_Files
|   |-- common.h           # Consolidates frequently used libraries.
|   |-- json_rpc.h         # JSON-RPC message utilities.
|   |-- websocket.h        # WebSocket session management.
|   |-- ws_net.h           
|   |-- utils.h            
|
|-- src
|   |-- main.cpp           # Entry point of the application.
|
|-- CMakeLists.txt         # Build configuration.
|-- README.md              

```

## Key Libraries

- Boost.Beast: A C++ library for handling HTTP and WebSocket communication, enabling efficient WebSocket connections and data transfer.

- Boost.Asio: An asynchronous I/O library that handles non-blocking network operations, providing scalability and improved performance.

- OpenSSL: A toolkit for SSL/TLS encryption, used to secure WebSocket connections and protect data during transmission.

- nlohmann/json: A lightweight C++ library for parsing and generating JSON, simplifying the handling of JSON data in API interactions.