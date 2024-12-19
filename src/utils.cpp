#include "utils.h"
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>

void validate_place_order(const po::variables_map& vm) {
    if (!vm.count("direction") || (vm["direction"].as<std::string>() != "buy" && vm["direction"].as<std::string>() != "sell")) {
        throw std::invalid_argument("Invalid or missing 'direction'. Must be 'buy' or 'sell'.");
    }

    if (!vm.count("instrument_name") || vm["instrument_name"].as<std::vector<std::string>>().empty()) {
        throw std::invalid_argument("Missing 'instrument_name'.");
    }
    if(vm["instrument_name"].as<std::vector<std::string>>().size() > 1){
        throw std::invalid_argument("Only one 'instrument_name' is allowed for the 'place' command.");
    }

    std::string order_type = "limit";
    if (vm.count("type")) {
        order_type = vm["type"].as<std::string>();
    }

    bool has_amount = vm.count("amount");
    bool has_contracts = vm.count("contracts");

    if (!has_amount && !has_contracts) {
        throw std::invalid_argument("At least one of 'amount' or 'contracts' must be provided.");
    }

    if (has_amount && has_contracts) {
        double amount = vm["amount"].as<double>();
        double contracts = vm["contracts"].as<double>();
        if (amount != contracts) {
            throw std::invalid_argument("'amount' and 'contracts' must match if both are provided.");
        }
    }

    if (order_type == "limit" || order_type == "stop_limit") {
        if (!vm.count("price") || vm["price"].as<double>() <= 0) {
            throw std::invalid_argument("Missing or invalid 'price'. Must be a positive number.");
        }
    }

    if (order_type == "stop_limit" || order_type == "stop_market" || order_type == "market") {
        if (order_type != "market" && (!vm.count("trigger_price") || vm["trigger_price"].as<double>() <= 0 || ((vm["trigger_price"].as<double>() * 10) -(int)(vm["trigger_price"].as<double>() * 10) != 0 ))) {
            throw std::invalid_argument("Missing or invalid 'trigger_price'. Must be a positive number with tick size 0.1.");
        }

        if (order_type == "stop_limit") {
            if (!vm.count("trigger") || (vm["trigger"].as<std::string>() != "index_price" && 
                                          vm["trigger"].as<std::string>() != "mark_price" && 
                                          vm["trigger"].as<std::string>() != "last_price")) {
                throw std::invalid_argument("Invalid or missing 'trigger'. Must be one of 'index_price', 'mark_price', or 'last_price'.");
            }
        }
    }

    if (vm.count("label") && vm["label"].as<std::string>().size() > 64) {
        throw std::invalid_argument("'label' must not exceed 64 characters.");
    }
}


jsonrpc store_required_values(const po::variables_map& vm){
    jsonrpc j;
    
    if (vm.count("direction")) {
        j["method"] = "private/" + vm["direction"].as<std::string>();
    }
    
    j["params"] = json::object();

    if (vm.count("instrument_name")) {
        j["params"].push_back({"instrument_name", vm["instrument_name"].as<std::vector<std::string>>()[0]});
    }

    if (vm.count("type")) {
        j["params"].push_back({"type", vm["type"].as<std::string>()});
    } else {
        j["params"].push_back({"type", "limit"});
    }

    if (vm.count("amount")) {
        j["params"].push_back({"amount", vm["amount"].as<double>()});
    }

    if (vm.count("contracts")) {
        j["params"].push_back({"contracts", vm["contracts"].as<double>()});
    }

    if (vm.count("price")) {
        j["params"].push_back({"price", vm["price"].as<double>()});
    }

    if (vm.count("trigger_price")) {
        j["params"].push_back({"trigger_price", vm["trigger_price"].as<double>()});
    }

    if (vm.count("trigger")) {
        j["params"].push_back({"trigger", vm["trigger"].as<std::string>()});
    }

    return j;
}

//configures help message options
po::options_description configure_help_options() {
    po::options_description desc("Available commands");
    desc.add_options()
        ("help,h", "Produce help message")
        ("connect", "Connect to Deribit WebSocket API and authenticate.")
        ("auth", "Authentication with the Deribit Test WebSocket API")
        ("place", 
         "Place a new order. Required parameters: \n"
         "   --direction <buy|sell>                (Required.\n"
         "   --instrument_name <string>            (Required).\n"
         "   --type <limit|stop_limit|market|stop_market>\n"
         "                         (Optional)Default: 'limit'.\n"
         "   --amount <positive double>(Atleast one).\n"
         "   --contracts <positive double>(Atleast one).\n"
         "           (If both provided, values must match.)\n"
         "   --price <positive double>\n"
         "            (Required for 'limit' and 'stop_limit').\n"
         "   --trigger_price <positive double>\n"
         "      (Required for 'stop_limit' and 'stop_market').\n"
         "                                     Tick size: 0.1.\n"
         "   --trigger <index_price|mark_price|last_price>\n"
         "                        (Required for 'stop_limit').\n"
         "   --label <string> \n"
         "            (Optional)Max length: 64 cPostharacters.\n"
         "   Additional options: \n"
         "      --post_only <bool>                 .\n"
         "      --reject_post_only <bool>          .\n"
         "      --mmp <bool>                       .\n"
         "      --valid_until <int>                \n"
         "      --trigger_offset <positive double> \n")
        ("cancel", "Cancel an order. Required parameters:\n"
         "   --order_id <string>")
        ("get_order_book", 
         "Get the order book for an instrument. Required parameters:\n"
         "   --instrument_name <string>\n"
         "   --depth <1|5|10|20|50|100|1000|10000>")
        ("subscribe", 
         "Subscribe to one or more instruments. Required parameters:\n"
         "   --channel <string>  --instrument_name <string>\n")
        ("unsubscribe_all", "Unsubscribe from all the instruments subscribed\n")
        ("exit", "Exit the program");

    return desc;
}


// Configures all CLI options
void configure_cmdline_options(po::options_description& desc) {
    desc.add_options()
        ("help,h", "Display help message")
        ("connect", "Connect to the Deribit WebSocket API")
        ("auth", "Performs authentication and saves tokens")
        ("exit", "Exit the program")
        ("place", "Place a new order")
        ("cancel", "Cancel an order")
        ("get_order_book", "Get the order book for an instrument")
        ("subscribe", "Subscribe to a channel for an instrument")
        ("order_id", po::value<std::string>(), "Order ID (required for cancel)")
        ("channel", po::value<std::vector<std::string>>()->multitoken(), "Channel name(s) (for subscribe)")
        ("instrument_name", po::value<std::vector<std::string>>()->multitoken(), "Instrument name(s) (for subscribe)")
        ("depth", po::value<int>(), "Depth for order book")
        ("direction", po::value<std::string>(), "Order direction ('buy' or 'sell')")
        ("type", po::value<std::string>()->default_value("limit"), "Order type ('limit', 'stop_limit', 'market', 'stop_market')")
        ("amount", po::value<double>(), "Order amount (positive number)")
        ("contracts", po::value<double>(), "Order contracts (positive number)")
        ("price", po::value<double>(), "Order price (positive number)")
        ("trigger_price", po::value<double>(), "Trigger price (positive number with tick size 0.1)")
        ("trigger", po::value<std::string>(), "Trigger type ('index_price', 'mark_price', 'last_price')")
        ("post_only", po::value<bool>(), "Post-only flag")
        ("reject_post_only", po::value<bool>(), "Reject post-only flag")
        ("mmp", po::value<bool>(), "Market Maker Protection flag")
        ("label", po::value<std::string>(), "Label (max 64 characters)")
        ("max_show", po::value<double>(), "Max show amount")
        ("valid_until", po::value<int>(), "Valid until timestamp")
        ("trigger_offset", po::value<double>(), "Trigger offset")
        ("unsubscribe_all", "Unsubscribe from all the channels subscribed");
}