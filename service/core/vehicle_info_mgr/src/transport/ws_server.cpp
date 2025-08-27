#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <iostream>
#include <thread>

typedef websocketpp::server<websocketpp::config::asio> server;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

// pull out the type of messages sent by our config
typedef server::message_ptr message_ptr;

void on_message(server* s, websocketpp::connection_hdl hdl, message_ptr msg) {
    std::cout << "on_message called with hdl: " << hdl.lock().get()
              << " and message: " << msg->get_payload()
              << std::endl;

    // TODO: Parse the incoming JSON-RPC message and forward to the request queue

    try {
        s->send(hdl, msg->get_payload(), msg->get_opcode());
    } catch (const websocketpp::lib::error_code& e) {
        std::cout << "Echo failed because: " << e
                  << "(" << e.message() << ")" << std::endl;
    }
}

void run_ws_server() {
    server ws_server;

    try {
        // Set logging settings
        ws_server.set_access_channels(websocketpp::log::alevel::all);
        ws_server.clear_access_channels(websocketpp::log::alevel::frame_payload);

        // Initialize Asio
        ws_server.init_asio();

        // Register our message handler
        ws_server.set_message_handler(bind(&on_message,&ws_server,::_1,::_2));

        // Listen on port 9090
        ws_server.listen(9090);

        // Start the server accept loop
        ws_server.start_accept();

        // Start the ASIO io_service run loop
        std::cout << "WebSocket server starting on port 9090" << std::endl;
        ws_server.run();
    } catch (websocketpp::exception const & e) {
        std::cout << e.what() << std::endl;
    } catch (...) {
        std::cout << "other exception" << std::endl;
    }
}
