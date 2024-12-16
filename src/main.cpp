#include <olc_net.h>

int main(int ac, char* av[]) {
    bool running = true;
    std::cout << "Welcome to the interactive CLI program! Type '--help' for options.\n";

    // The io_context is required for all I/O
    net::io_context ioc;
    const int ioc_threads = 1;
    std::vector<std::thread> ioc_thread_pool;

    // for signal handler to exit cleanly
    boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait([&](auto, auto){
        ioc.stop();
        running=false;
        std::cout << "Signal received, stopped the process.";
    });

    // The SSL context is required, and holds certificates
    ssl::context ctx{ssl::context::tlsv12_client};
    // This holds the root certificate used for verification
    ctx.set_default_verify_paths();

    // Shared pointer to manage WebSocket session
    std::shared_ptr<session> ws_session;

    // Start the io_context in multiple threads
    for (int i = 0; i < ioc_threads; ++i) {
        ioc_thread_pool.emplace_back([&ioc] {
            ioc.run();
        });
    }

    while (running) {
        try {
            std::string input_line;
            std::cout << "> "; // Prompt for input
            std::getline(std::cin, input_line); // Get user input

            if(input_line.empty()) {
                continue; // Skip empty input
            }

            if(input_line == "--help") {
                //print all options
                std::cout << "Available commands:\n";
                std::cout << "  deribit --connect       - Connect to Deribit WebSocket API and authenticate.\n";
                std::cout << "  exit                 - Exit the program.\n";
            }else if (input_line == "deribit --connect") {
                if (ws_session) {
                    std::cout << "Already connected to Deribit. Please disconnect before reconnecting.\n";
                    continue;
                }

                ws_session = std::make_shared<session>(ioc, ctx);

                jsonrpc j;
                j["method"] = "public/auth";
                j["params"] = {
                    {"grant_type", "client_credentials"},
                    {"client_id", "5LlATT7V"},
                    {"client_secret", "p9hjUS1ohjMnXDt9yMVa7qZiODeAf-IWn943Zs0BOFU"}
                };
                std::string auth_message = j.dump();

                ws_session->run("test.deribit.com", "443", auth_message.c_str(), "/ws/api/v2");
            }else if(input_line.substr(0,15) == "deribit --place"){
                //deribit --place direction=buy instrument=BTC-USD type=limit amount=0.5 price=50000
                if (ws_session && !ws_session->get_access_token().empty()) {
                    std::string param_string = input_line.substr(16);

                    std::unordered_map<std::string, std::string> params;

                    // Parse the parameters
                    std::istringstream iss(param_string);
                    std::string token;
                    while (std::getline(iss, token, ' ')) {
                        size_t eq_pos = token.find('=');
                        
                        if (eq_pos != std::string::npos && eq_pos < token.size() - 1) {  // Ensure '=' exists and there's a value
                            std::string key = token.substr(0,eq_pos);
                            std::string value = token.substr(eq_pos + 1);
                            params[key] = value;
                        } else {
                            std::cerr << "Error: Malformed parameter '" << token << "'. Expected format is key=value.\n";
                            return 1;
                        }
                    }

                    jsonrpc j;

                    if (params.find("direction") == params.end()) {
                        std::cout << "Error: 'direction' is a mandatory parameter.\n";
                        continue;
                    }
                    j["method"] = "private/" + params["direction"];
                    j["params"] = json::object();
                    if (params.find("instrument") == params.end()) {
                        std::cout << "Error: 'instrument' is a mandatory parameter.\n";
                        continue;
                    }
                    
                    j["params"].push_back({"instrument", params["instrument"]});

                    if (params.find("type") == params.end()) {
                        std::cout << "Error: 'type' is a mandatory parameter.\n";
                        continue;
                    }

                    // Additional validations based on type
                    std::string type = params["type"];

                    if (type == "market" || type == "stop_market" || type == "take_market") {
                        if (params.find("amount") == params.end() || params.find("contracts") == params.end()) {
                            std::cout << "Error: Either 'amount' or 'contracts' is required for market orders.\n";
                            continue;
                        }
                    } else if (type == "limit" || type == "stop_limit" || type == "take_limit" || type == "market_limit") {
                        if (params.find("amount") == params.end() && params.find("contracts") == params.end()) {
                            std::cout << "Error: Either 'amount' or 'contracts' is required for limit orders.\n";
                            continue;
                        }
                        if (params.find("price") == params.end()) {
                            std::cout << "Error: 'price' is required for limit orders.\n";
                            continue;
                        }
                    } else if (type == "trailing_stop") {
                        if (params.find("trigger_offset") == params.end()) {
                            std::cout << "Error: 'trigger_offset' is required for trailing stop orders.\n";
                            continue;
                        }
                    } else if (type == "stop_market" || type == "stop_limit") {
                        if (params.find("trigger_price") == params.end()) {
                            std::cout << "Error: 'trigger_price' is required for stop orders.\n";
                            continue;
                        }
                    }

                     // Validate numerical values if present
                    try {
                        if (params.find("amount") != params.end()) {
                            double amount = std::stod(params["amount"]);
                            if (amount <= 0) {
                                throw std::invalid_argument("Amount must be positive.");
                            }
                        }

                        if (params.find("contracts") != params.end()) {
                            double contracts = std::stod(params["contracts"]);
                            if (contracts <= 0) {
                                throw std::invalid_argument("Contracts must be positive.");
                            }
                        }

                        if (params.find("price") != params.end()) {
                            double price = std::stod(params["price"]);
                            if (price <= 0) {
                                throw std::invalid_argument("Price must be positive.");
                            }
                        }

                        if (params.find("trigger_price") != params.end()) {
                            double trigger_price = std::stod(params["trigger_price"]);
                            if (trigger_price <= 0) {
                                throw std::invalid_argument("Trigger price must be positive.");
                            }
                        }

                        if (params.find("trigger_offset") != params.end()) {
                            double trigger_offset = std::stod(params["trigger_offset"]);
                            if (trigger_offset <= 0) {
                                throw std::invalid_argument("Trigger offset must be positive.");
                            }
                        }
                    } catch (const std::invalid_argument &e) {
                        std::cout << "Error: " << e.what() << "\n";
                        continue;
                    }

                    for (const auto& [key, value] : params) {
                        std::cout << key << ": " << value << "\n";
                    }
                    // jsonrpc j;
                    // j["method"] = "private/" + params["direction"];
                    // j["params"] = {
                    //     {"instrument_name", ""},
                    //     {"amount", 40},
                    //     {"type", "market"},
                    //     {"label", "market0000234"}
                    // };

                    // std::string message = j.dump();
                    // ws_session->send_message(message); // Send the message
                    // std::cout << "Placed order request sent.\n";
                } else {
                    std::cout << "Error: Access token not set. Please authenticate first.\n";
                }
            }else if(input_line == "deribit --cancel"){
                //by order id.
            }
            //add subscribe and unsubscribe (symbols handling)
            //view current positions
            //continuous orderbook updates for subscribed symbols 
            //get order book
            else if (input_line == "exit"){
                if (ws_session) {
                    ws_session->close_websocket();
                    ws_session.reset(); // Reset the session to clean up resources
                }

                std::cout << "Exiting the program...\n";
                running = false;
                ioc.stop();
            }else {
                std::cout << "Unknown command. Type '--help' for options.\n";
            }
        } catch (const std::exception& e) {
            std::cout << "Error: " << e.what() << "\n";
        }
    }

    for (auto& t : ioc_thread_pool) {
        if (t.joinable()) {
            t.join();
        }
    }

    return 0;
}
