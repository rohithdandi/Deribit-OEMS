#pragma once
#include "json_rpc.h"
#include "common.h"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
typedef boost::asio::io_context::executor_type executor_type;
typedef boost::asio::strand<executor_type> strand;

// Report a failure
void fail(beast::error_code ec, char const* what){
    std::cerr << what << ": " << ec.message() << "\n";
}

// Sends a WebSocket message and prints the response
// enabled_shared_from_this provided function shared_from_this
// which is a handy function to create shared pointers from this
class session : public std::enable_shared_from_this<session> {
private:
    net::io_context& ioc_;
    tcp::resolver resolver_;
    websocket::stream<beast::ssl_stream<beast::tcp_stream>> ws_;
    beast::flat_buffer buffer_;
    std::string host_;
    std::string text_;
    std::string endpoint_;
    strand ws_strand_;
    std::string access_token_;
    std::unordered_map<std::string, double> subscribed_symbols_;
    std::mutex data_mutex;

public:
    // Resolver and socket require an io_context
    explicit
    session(net::io_context& ioc, ssl::context& ctx)
        : resolver_(net::make_strand(ioc)) // Looks up the domain name
        , ws_(net::make_strand(ioc), ctx) // Websocket constructor takes an IO context and ssl context
        , ioc_(ioc) // reference to the io_context created in the main function
        , ws_strand_(ioc.get_executor()) // Get a strand from the io_context
    {

    }

    // Start the asynchronous operation
    void run(
        char const* host,
        char const* port,
        char const* text,
        char const* endpoint)
    {
        // Save these for later
        host_ = host;
        text_ = text;
        endpoint_ = endpoint;

        std::cout << "Resolving domain name: " << host << " on port: " << port << std::endl;
        // Look up the domain name
        resolver_.async_resolve(
            host,
            port,
            // Binds parameters to the handler creating a new handler
            beast::bind_front_handler(
                &session::on_resolve,
                shared_from_this()));
    }

    void on_resolve(beast::error_code ec, tcp::resolver::results_type results){
        if(ec)
            return fail(ec, "resolve");

        std::cout << "Domain name resolved. Attempting to establish a connection..." << std::endl;
        // Set a timeout on the operation
        // Get lowest layer will get the underlying socket
        //websocket::stream<beast::ssl_stream<beast::tcp_stream>> ws_;
        beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

        // Make the connection on the IP address we get from a lookup
        beast::get_lowest_layer(ws_).async_connect(
            results,
            // Binds the strand to an executor of the type the strand is based on, in this case io_context
            // bind_front_handler creates a functor object which when execute calls on_connect function of this pointer i.e
            // current instance of session.
            // Once Async_connect completes it calls the functor object with error_code ec and endpoint_type ep
            // which are required arguments to call on_connect function
            boost::asio::bind_executor(ws_strand_, beast::bind_front_handler( &session::on_connect, shared_from_this())));
            // beast::bind_front_handler(
            //     &session::on_connect,
            //     shared_from_this()));
    }

    void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep){
        if(ec)
            return fail(ec, "connect");

        std::cout << "Connection established with endpoint: " << ep.address() << ":" << ep.port() << std::endl;
        std::cout << "Starting SSL handshake..." << std::endl;
        // Gets the socket associated with this web socket and sets a timeout on the operation.
        // ws_ is declared as websocket::stream<beast::ssl_stream<beast::tcp_stream>>
        // get_lower_layer will return beast::tcp_stream
        beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

        // Set SNI Hostname (many hosts need this to handshake successfully)
        if(! SSL_set_tlsext_host_name(
                ws_.next_layer().native_handle(),
                host_.c_str()))
        {
            ec = beast::error_code(static_cast<int>(::ERR_get_error()),
                net::error::get_ssl_category());
            return fail(ec, "connect");
        }

        // Update the host_ string. This will provide the value of the
        // Host HTTP header during the WebSocket handshake.
        // See https://tools.ietf.org/html/rfc7230#section-5.4
        host_ += ':' + std::to_string(ep.port());

        // Perform the SSL handshake.
        // ws_ is declared as websocket::stream<beast::ssl_stream<beast::tcp_stream>>
        // next_layer will return beast::ssl_stream
        ws_.next_layer().async_handshake(
            ssl::stream_base::client,
            boost::asio::bind_executor(ws_strand_, beast::bind_front_handler( &session::on_ssl_handshake, shared_from_this())));
            // beast::bind_front_handler(
            //     &session::on_ssl_handshake,
            //     shared_from_this()));
    }

    void on_ssl_handshake(beast::error_code ec){
        if(ec)
            return fail(ec, "ssl_handshake");

        std::cout << "SSL Handshake successful. Proceeding to WebSocket handshake..." << std::endl;
        // Turn off the timeout on the tcp_stream, because
        // the websocket stream has its own timeout system.
        beast::get_lowest_layer(ws_).expires_never();

        // Set suggested timeout settings for the websocket
        ws_.set_option(
            websocket::stream_base::timeout::suggested(
                beast::role_type::client));

        // Set a decorator to change the User-Agent of the handshake
        ws_.set_option(websocket::stream_base::decorator(
            [](websocket::request_type& req)
            {
                req.set(http::field::user_agent,
                    std::string(BOOST_BEAST_VERSION_STRING) +
                        " websocket-client-async-ssl");
            }));

        // Perform the websocket handshake
        ws_.async_handshake(host_, endpoint_,
            boost::asio::bind_executor(ws_strand_, beast::bind_front_handler( &session::on_handshake, shared_from_this())));
            // beast::bind_front_handler(
            //     &session::on_handshake,
            //     shared_from_this()));
    }

    void on_handshake(beast::error_code ec){
        if(ec)
            return fail(ec, "handshake");

        std::cout << "WebSocket Handshake successful. Sending authentication message..." << std::endl;

        // Send the message
        ws_.async_write(
            net::buffer(text_),
            boost::asio::bind_executor(ws_strand_, beast::bind_front_handler( &session::on_write, shared_from_this())));
            // beast::bind_front_handler(
            //     &session::on_write,
            //     shared_from_this()));
    }

    void on_write(beast::error_code ec, std::size_t bytes_transferred){
        // Supress unused variable compiler warnings
        // bytes_transferred contains total bytes read/written to the websocket stream
        boost::ignore_unused(bytes_transferred);

        if(ec)
            return fail(ec, "write");

        std::cout << "Authentication message sent. Awaiting response..." << std::endl;

        // Read single message
        ws_.async_read(
            buffer_,
            boost::asio::bind_executor(ws_strand_, beast::bind_front_handler( &session::on_read, shared_from_this())));
    }

    void set_access_token(const std::string& token) {
        access_token_ = token;
    }

    // Function to get the access token
    std::string get_access_token() const {
        return access_token_;
    }

    std::unordered_map<std::string, double> get_subscribed_symbols() const {
        return subscribed_symbols_;
    }

    void on_read(beast::error_code ec, std::size_t bytes_transferred){
        boost::ignore_unused(bytes_transferred);

        if(ec){
            fail(ec, "read");
            close_websocket(); // Gracefully close the connection on read error
            return;
        }

        // Print the messages
        // make_printable interprets the bytes are characters and sends to output stream
        // do not print subscribed symbol updates.

        std::string response = boost::beast::buffers_to_string(buffer_.data());

        // Clear the buffer
        buffer_.consume(buffer_.size());

        try {
            json j = json::parse(response);
            auto method_it = j.find("method");
            if (method_it != j.end() && *method_it == "subscription") {
                process_subscription(j);
            }else if (access_token_.empty()) {
                handle_access_token(j);
            }else{
                std::cout << "Received response from server " << "\n";
                std::cout << response << " by thread ID:" << boost::this_thread::get_id() << std::endl;
            }
        } catch (const json::parse_error& e) {
            std::cerr << "JSON parse error: " << e.what() << "\n";
        }

        ws_.async_read(
            buffer_,
            boost::asio::bind_executor(ws_strand_, beast::bind_front_handler( &session::on_read, shared_from_this())));
    }

    void process_subscription(const json& j) {
        try {
            std::lock_guard<std::mutex> lock(data_mutex);
            subscribed_symbols_[j["params"]["data"]["index_name"]] = j["params"]["data"]["price"];
        } catch (const std::exception& e) {
            std::cerr << "Error processing subscription: " << e.what() << "\n";
        }
    }

    void handle_access_token(const json& j) {
        if (j.contains("result") && j["result"].contains("access_token")) {
            set_access_token(j["result"]["access_token"]);
            std::cout << "Access token set successfully.\n";
        } else {
            std::cerr << "Error: No access token found in response.\n";
        }
    }

    void send_message(const std::string& message) {
    // Simply send the message asynchronously, without binding to on_write
        ws_.async_write(
            net::buffer(message),
            [](beast::error_code ec, std::size_t bytes_transferred) {
                if (ec) {
                    std::cerr << "Error sending message: " << ec.message() << std::endl;
                }
                else {
                    std::cout << "Sent message, bytes transferred: " << bytes_transferred << std::endl;
                }
            });
    }

    void close_websocket() {
        ws_.async_close(websocket::close_code::normal,
            boost::asio::bind_executor(ws_strand_, beast::bind_front_handler(
                &session::on_close, shared_from_this())));
    }

    // when a signal handler is triggered
    void on_close(beast::error_code ec){
        if(ec)
            return fail(ec, "close");

        // If we get here then the connection is closed gracefully
        // The make_printable() function helps print a ConstBufferSequence
        std::cout << beast::make_printable(buffer_.data()) << std::endl;
    }
};
