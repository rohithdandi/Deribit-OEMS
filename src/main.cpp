#include <ws_net.h>
#include "utils.h"


int main(int ac, char* av[]) {
    std::atomic<bool> running(true);
    po::options_description desc = configure_help_options();

    std::atomic<bool> show_subscriptions(false);
    std::cout << "Welcome to the interactive CLI program! Type '--help' for options.\n";

    // The io_context is required for all I/O
    net::io_context ioc;
    const int ioc_threads = 2;
    std::vector<std::thread> ioc_thread_pool;

    // for signal handler to exit cleanly
    boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait([&](auto, auto){
        ioc.stop();
        running.store(false, std::memory_order_release);
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
            po::options_description cmdline_options("Available inputs");
            configure_cmdline_options(cmdline_options);

            std::string input_line;
            std::cout << "\\Deribit > $"; // Prompt for input
            if (!std::getline(std::cin, input_line)) {  // Check for EOF or errors
                std::cerr << "\nInput stream closed. Exiting command loop...\n";
                running = false;
                ioc.stop();
                break;
            }

            if(input_line.empty()) {
                continue; // Skip empty input
            }

            std::vector<std::string> args = po::split_unix(input_line);
            args.insert(args.begin(), "program"); // Fake program name for parsing

            po::variables_map vm;
            po::store(po::command_line_parser(args).options(cmdline_options).run(), vm);
            po::notify(vm);

            if(vm.count("help")) {
                std::cout << desc << "\n";
            }else if (vm.count("connect")) {
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
            } else if(vm.count("place")){
                //--place direction=<> instrument_name=<> type=<> 
                //--place --direction buy --instrument_name ETH-PERPETUAL --type limit --amount 40 --price 1500
                //--place --direction sell --instrument_name ETH-PERPETUAL --type stop_limit --amount 10 --price 145.6 --trigger_price 145 --trigger last_price
                if (!ws_session || ws_session->get_access_token().empty()) {
                    std::cout << "Error: Access token not set. Please authenticate first.\n";
                    continue;
                }
                validate_place_order(vm);
                jsonrpc j = store_required_values(vm);
                std::string message = j.dump();
                ws_session->send_message(message); // Send the message
                std::cout << "Place order request sent.\n";
            }else if(vm.count("cancel")){
                //--cancel --order_id ETH-SLIS-12
                if (!ws_session || ws_session->get_access_token().empty()) {
                    std::cout << "Error: Access token not set. Please authenticate first.\n";
                    continue;
                }
                //by order id.
                if (!vm.count("order_id")) {
                    throw std::invalid_argument("Missing required parameter for cancel: --order_id.");
                }
                jsonrpc j("/private/cancel");
                j["params"] = {
                    {"order_id", vm["order_id"].as<std::string>()},
                };
                std::string message = j.dump();
                ws_session->send_message(message);
                std::cout << "cancel order request sent.\n";
            }else if(vm.count("get_order_book")){
                std::unordered_set <int> valid_depths = {1,5,10,20,50,100,1000,10000};
                if (!vm.count("instrument_name") || !vm.count("depth")) {
                    throw std::invalid_argument("Missing required parameters for get_order_book: --instrument_name and --depth.");
                }
                if(valid_depths.find(vm["depth"].as<int>()) == valid_depths.end()){
                    throw std::invalid_argument("Depth has an invalid value. valid depths are {1,5,10,20,50,100,1000,10000}");
                }

                jsonrpc j("public/get_order_book");
                j["params"] = {
                    {"instrument_name", vm["instrument_name"].as<std::string>()},
                    {"depth", vm["depth"].as<int>()},
                };

                std::string message = j.dump();
                std::cout << message << "\n\n";
                ws_session->send_message(message); // Send the message
                std::cout << "Placed get order book request sent.\n";
            }else if(vm.count("subscribe")){
                //--subscribe --channel deribit_price_index --instrument_name btc_usd
                // only one subscription per command

                if (!ws_session || ws_session->get_access_token().empty()) {
                    std::cout << "Error: Access token not set. Please authenticate first.\n";
                    continue;
                }
                
                if (!vm.count("instrument_name") || !vm.count("channel")) {
                    throw std::invalid_argument("Missing required parameters for subscribe: --instrument_name and --channel.");
                }
                jsonrpc j("private/subscribe");
                j["params"] = {
                    {"channels", {vm["channel"].as<std::string>()+"."+vm["instrument_name"].as<std::string>()}},
                };

                std::string message = j.dump();
                std::cout << message << "\n\n";
                ws_session->send_message(message); // Send the message
                std::cout << "subscribe request sent.\n";
            }else if(vm.count("unsubscribe_all")){
                if (!ws_session || ws_session->get_access_token().empty()) {
                    std::cout << "Error: Access token not set. Please authenticate first.\n";
                    continue;
                }

                jsonrpc j("private/unsubscribe_all");
                std::string message = j.dump();
                ws_session->send_message(message); // Send the message
                std::cout << "unsubscribe all request sent.\n";
            }
            // else if (vm.count("show_subscribed")) {
            //     show_subscriptions.store(true);
            //     // Launch a background thread to print the subscribed symbols to a file
            //     std::thread subscription_thread(print_subscribed_symbols_to_file, ws_session, std::ref(show_subscriptions));
            //     subscription_thread.detach(); // Detach the thread to run independently
            //     std::cout << "Displaying subscribed symbols. Type 'deribit --stop_showing_subscribed' to stop.\n";
            // }
            // else if (vm.count("stop_subscribed")) {
            //     show_subscriptions.store(false);
            //     std::cout << "Stopped displaying subscribed symbols.\n";
            // }
            else if (vm.count("exit")){
                if (ws_session) {
                    std::cout << "Closing WebSocket connection...\n";
                    ws_session->close_websocket();
                }

                std::cout << "Exiting the program...\n";
                running.store(false, std::memory_order_release);
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