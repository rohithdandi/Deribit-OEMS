#pragma once
#include "json.hpp"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

using json = nlohmann::json;

class jsonrpc : public json {
private:
    // Generate a UUID using boost::uuid. ensures Thread safety
    static std::string generate_uuid() {
        static boost::uuids::random_generator uuid_gen;
        boost::uuids::uuid id = uuid_gen();
        return boost::uuids::to_string(id);
    }

public:
    jsonrpc() {
        (*this)["jsonrpc"] = "2.0";
        (*this)["id"] = generate_uuid();
    }

    jsonrpc(const std::string& method) {
        (*this)["jsonrpc"] = "2.0";
        (*this)["method"] = method;
        (*this)["id"] = generate_uuid();
    }
};