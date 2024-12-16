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
                //deribit --place direction=<> instrument_name=<> type=<> 
                //deribit --place direction=buy instrument_name=ETH-PERPETUAL type=market amount=40 label=market0000234
                if (ws_session && !ws_session->get_access_token().empty()) {

                    if (input_line.size() <= 17) {
                        std::cout << "Error: No parameters provided. Expected parameters in the format key=value.\n";
                        continue;
                    }
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
                            continue;
                        }
                    }

                    jsonrpc j;

                    if (params.find("direction") == params.end()) {
                        std::cout << "Error: 'direction' is a mandatory parameter.\n";
                        continue;
                    }

                    if (params.find("instrument_name") == params.end()) {
                        std::cout << "Error: 'instrument_name' is a mandatory parameter.\n";
                        continue;
                    }

                    if (params.find("type") == params.end()) {
                        //limit order by default
                        params["type"]="limit";
                    }

                    j["method"] = "private/" + params["direction"];
                    j["params"] = json::object();
                    j["params"].push_back({"instrument_name", params["instrument_name"]});
                    j["params"].push_back({"type", params["type"]});

                    // Additional validations based on type
                    std::string type = params["type"];
                    auto validate_amount_contracts = [&]() -> bool {
                        if (params.find("amount") == params.end() && params.find("contracts") == params.end()) {
                            std::cerr << "Error: Either 'amount' or 'contracts' is required for " << type << " order.\n";
                            return false;
                        }
                        if (params.find("amount") != params.end() && params.find("contracts") != params.end() && params["amount"] != params["contracts"]) {
                            std::cerr << "Error: If both 'contracts' and 'amount' are passed, they must match each other.\n";
                            return false;
                        }
                        if (params.find("amount") != params.end()) {
                            j["params"].push_back({"amount", std::stod(params["amount"])});
                        }
                        if (params.find("contracts") != params.end()) {
                            j["params"].push_back({"contracts", std::stod(params["contracts"])});
                        }
                        return true;
                    };

                    if (type == "limit" || type == "stop_limit") {
                        if (!validate_amount_contracts()) continue;

                        if (params.find("price") == params.end()) {
                            std::cerr << "Error: 'price' is required for " << type << " order.\n";
                            continue;
                        }
                        j["params"].push_back({"price", std::stod(params["price"])});

                        if (type == "stop_limit") {
                            if (params.find("trigger_price") == params.end()) {
                                std::cerr << "Error: 'trigger_price' is required for stop_limit order.\n";
                                continue;
                            }
                            j["params"].push_back({"trigger_price", std::stod(params["trigger_price"])});

                            if (params.find("trigger") == params.end()) {
                                std::cerr << "Error: 'trigger' is required for stop_limit order.\n";
                                continue;
                            }
                            j["params"].push_back({"trigger", params["trigger"]});
                        }
                    } else if (type == "market" || type == "stop_market") {
                        if (!validate_amount_contracts()) continue;

                        if (type == "stop_market" && params.find("trigger_price") == params.end()) {
                            std::cerr << "Error: 'trigger_price' is required for stop_market order.\n";
                            continue;
                        }
                        if (params.find("trigger_price") != params.end()) {
                            j["params"].push_back({"trigger_price", std::stod(params["trigger_price"])});
                        }
                    } else {
                        std::cerr << "Error: Unsupported order type '" << type << "'.\n";
                        continue;
                    }

                     // Validate numerical values if present
                    try {
                        auto validate_positive_number = [&](const std::string &key) {
                            if (params.find(key) != params.end()) {
                                double value = std::stod(params[key]);
                                if (value <= 0) {
                                    throw std::invalid_argument(key + " must be positive.");
                                }
                            }
                        };

                        auto validate_boolean = [&](const std::string &key) {
                            if (params.find(key) != params.end()) {
                                std::string value = params[key];
                                if (value != "true" && value != "false") {
                                    throw std::invalid_argument(key + " must be a boolean ('true' or 'false').");
                                }
                            }
                        };

                        auto validate_enum = [&](const std::string &key, const std::set<std::string> &valid_values) {
                            if (params.find(key) != params.end()) {
                                if (valid_values.find(params[key]) == valid_values.end()) {
                                    throw std::invalid_argument(key + " has an invalid value.");
                                }
                            }
                        };

                        validate_positive_number("amount");
                        validate_positive_number("contracts");
                        validate_positive_number("price");
                        validate_positive_number("trigger_price");
                        validate_positive_number("trigger_offset");
                        validate_boolean("post_only");
                        validate_boolean("reject_post_only");
                        validate_boolean("reduce_only");
                        validate_boolean("mmp");
                        validate_enum("time_in_force", {"good_til_cancelled", "good_til_day", "fill_or_kill", "immediate_or_cancel"});
                        validate_enum("trigger", {"index_price", "mark_price", "last_price"});
                        validate_enum("linked_order_type", {"one_triggers_other", "one_cancels_other", "one_triggers_one_cancels_other"});

                        if (params.find("label") != params.end() && params["label"].size() > 64) {
                            throw std::invalid_argument("label must not exceed 64 characters.");
                        }
                    } catch (const std::invalid_argument &e) {
                        std::cerr << "Error: " << e.what() << "\n";
                        continue;
                    }

                    // Add optional parameters based on type
                    if (type == "limit" || type == "stop_limit") {
                        if (params.find("post_only") != params.end()) {
                            j["params"].push_back({"post_only", params["post_only"] == "true"});
                        }
                        if (params.find("reject_post_only") != params.end() && params["post_only"] == "true") {
                            j["params"].push_back({"reject_post_only", params["reject_post_only"] == "true"});
                        }
                        if (params.find("mmp") != params.end()) {
                            j["params"].push_back({"mmp", params["mmp"] == "true"});
                        }
                    }

                    if (params.find("reduce_only") != params.end()) {
                        j["params"].push_back({"reduce_only", params["reduce_only"] == "true"});
                    }

                    if (params.find("time_in_force") != params.end()) {
                        j["params"].push_back({"time_in_force", params["time_in_force"]});
                    }

                    if (type == "stop_limit" || type == "stop_market") {
                        if (params.find("trigger") != params.end()) {
                            j["params"].push_back({"trigger", params["trigger"]});
                        }
                        if (params.find("trigger_price") != params.end()) {
                            j["params"].push_back({"trigger_price", std::stod(params["trigger_price"])});
                        }
                    }

                    // Add universal optional parameters
                    if (params.find("label") != params.end()) {
                        j["params"].push_back({"label", params["label"]});
                    }
                    if (params.find("max_show") != params.end()) {
                        j["params"].push_back({"max_show", std::stod(params["max_show"])});
                    }
                    if (params.find("valid_until") != params.end()) {
                        j["params"].push_back({"valid_until", std::stol(params["valid_until"])});
                    }
                    if (params.find("trigger_offset") != params.end()) {
                        j["params"].push_back({"trigger_offset", std::stod(params["trigger_offset"])});
                    }

                    // for (const auto& [key, value] : params) {
                    //     std::cout << key << ": " << value << "\n";
                    // }

                    std::string message = j.dump();
                    std::cout << message << "\n\n";
                    ws_session->send_message(message); // Send the message
                    std::cout << "Placed order request sent.\n";
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
