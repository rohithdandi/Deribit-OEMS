# CLI Utility for Deribit WebSocket API

This project provides a high-performance order execution and management system to trade on Deribit Test (https://test.deribit.com/) using Deribit WebSocket API. The CLI utility allows users to place orders, retrieve order books, subscribe to instruments, and more, all via simple command-line options.


![alt text](image.png)


## Getting Started

To explore the available options, type:

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

!!!WORK IN PROGRESS!!!