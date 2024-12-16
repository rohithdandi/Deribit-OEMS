#include <olc_net.h>

int main(int ac, char* av[]) {
    bool running = true;
    std::cout << "Welcome to the interactive CLI program! Type '--help' for options.\n";

    auto const threads = 1;
    std::vector<std::thread> v;
    
    // The io_context is required for all I/O
    net::io_context ioc;

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

    while (running) {
        try {
            std::string input_line;
            std::cout << "> "; // Prompt for input
            std::getline(std::cin, input_line); // Get user input

            if(input_line.empty()) {
                continue; // Skip empty input
            }

            if(input_line == "help") {
                //print all options
                std::cout << "Available commands:\n";
                std::cout << "  connect deribit       - Connect to Deribit WebSocket API and authenticate.\n";
                std::cout << "  exit                 - Exit the program.\n";
            }else if (input_line == "connect deribit") {
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
                
                v.reserve(threads - 1);
                for(auto i = threads - 1; i > 0; --i)
                    // Create an instance in place at the end of the vector
                    v.emplace_back(
                    [&ioc]
                    {
                        ioc.run();
                    });
                ioc.run();
            }
            //add subscribe and unsubscribe (symbols handling)
            //get order book
            //buy sell & cancel && editbylabel
            else if (input_line == "exit"){
                if (ws_session) {
                    ws_session->close_websocket();
                    ws_session.reset(); // Reset the session to clean up resources
                }

                for(auto& t : v)
                    t.join();

                std::cout << "Exiting the program...\n";
                running = false;
            }else {
                std::cout << "Unknown command. Type '--help' for options.\n";
            }
        } catch (const std::exception& e) {
            std::cout << "Error: " << e.what() << "\n";
        }
    }

    for(auto& t : v)
        t.join();

    return 0;
}
