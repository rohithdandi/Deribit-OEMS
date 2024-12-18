# Order Execution and Management System for Deribit Test API

## Overview

This project is a high-performance Command-Line Interface (CLI) application that facilitates order execution and management on the Deribit Test WebSocket API. Designed for efficient trading, the system employs modern C++ standards and powerful libraries to ensure robustness, scalability, and ease of use.

- To explore the available options, type:

```bash
\Deribit > $--help
```

## Available Commands

### General Commands
- **`--help`**  
  Produce help message.

- **`--connect`**  
  Connect to the Deribit WebSocket API and authenticate.

### Order Management
- **`--place`**  
  Place a new order. Required parameters:
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

- **`--cancel`**  
  Cancel an order. Required parameters:
  - **`--order_id <string>`**

### Market Data Commands
- **`--get_order_book`**  
  Get the order book for an instrument. Required parameters:
  - **`--instrument_name <string>`**
  - **`--depth <1|5|10|20|50|100|1000|10000>`**

- **`--subscribe`**  
  Subscribe to an instrument. Required parameters:
  - **`--instrument_name <string>`**
  - **`--channel <string>`**

- **`--unsubscribe_all`**  
  Unsubscribe from all the instruments currently subscribed.

- **`--show_subscribed`**  
  Display real-time data for subscribed symbols.

- **`--stop_subscribed`**  
  Stop displaying real-time data for subscribed symbols.

### Program Control
- **`--exit`**  
  Exit the program.

---

![alt text](image.png)


## Features

- WebSocket Connectivity: Real-time communication with the Deribit Test WebSocket API.

- Order Management: Place, cancel, and query orders.

- Subscription Management: Subscribe and unsubscribe from market data feeds.

- Multi-threaded I/O: Efficient handling of I/O operations using Boost Asio.

- Signal Handling: Graceful termination on receiving system signals.

- Configuration: Flexible command-line options for customization.

## Project Structure

```sh
.
|-- Header_Files
|   |-- common.h           # Consolidates frequently used libraries.
|   |-- json_rpc.h         # JSON-RPC message utilities.
|   |-- websocket.h        # WebSocket session management.
|   |-- ws_net.h           
|
|-- src
|   |-- main.cpp           # Entry point of the application.
|   |-- utils.h            # Helper functions declarations.
|   |-- utils.cpp          # Helper functions implementation.
|
|-- CMakeLists.txt         # Build configuration.
|-- README.md              

```

## Implementation Details

## Architecture

1.Core Components:

- WebSocket Session:
Manages the lifecycle of a WebSocket connection, including connection, authentication, message sending, and reception.

- JSON-RPC Utilities:
Encodes and decodes messages to and from the JSON-RPC format required by Deribit API.

- CLI Command Parser:
Parses user input to execute corresponding actions.

2.Concurrency:

- Utilizes Boost Asio’s io_context to handle asynchronous I/O operations.

- Multi-threaded design ensures non-blocking communication.

3.Signal Handling:

- Gracefully shuts down the application when receiving SIGINT or SIGTERM signals.

4.Helper Utilities:

utils.h and utils.cpp:

Command-line Options: Functions like configure_cmdline_options and configure_help_options define and manage CLI options using Boost Program Options.

Input Validation: The validate_place_order function ensures user inputs like order parameters are correctly formatted.

Storing Values: The store_required_values function encapsulates validated inputs into a JSON-RPC request format for seamless integration with the WebSocket layer.

5.Header Modules:

common.h: Consolidates frequently used standard and Boost libraries for consistency across all modules. Simplifies maintainability and ensures thread-safety for concurrent operations.

json_rpc.h: Provides a clean and thread-safe implementation of JSON-RPC requests, leveraging Boost UUID for unique IDs in a multi-threaded environment.

websocket.h: Implements the session class for WebSocket connections, handling lifecycle events like on_resolve, on_connect, and on_read. Uses Boost strands and mutexes to ensure thread safety during message processing.

## Commands and Functionality

- **--connect**
Establishes a WebSocket connection with Deribit Test API and authenticates using client credentials.

- **--place**
Places an order with options like direction, instrument name, type, amount, price, etc. Validates inputs using utils.h functions before processing.

- **--cancel**
Cancels an order by order ID. Ensures only valid formats are processed.

- **--get_order_book**
Fetches the order book for a specified instrument and depth.

- **--subscribe / --unsubscribe_all**
Manages subscriptions to market data channels.

- **--show_subscribed / --stop_subscribed**
Displays active subscriptions in real-time.

## Key Libraries

- Boost Asio: Asynchronous I/O for WebSocket and signal handling.

- Boost Program Options: Parsing and validating command-line arguments.

- nlohmann::json: Convenient handling of JSON for API requests and responses.

- Boost UUID: Ensures thread-safe generation of unique IDs for JSON-RPC requests.

## Further Improvements


!!!WORK IN PROGRESS!!!