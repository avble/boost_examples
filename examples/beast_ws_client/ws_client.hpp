#pragma once

#include "config.hpp"

class session_client;

class ws_client_writer: public std::enable_shared_from_this<ws_client_writer>{

    enum ws_write_state {none = 0, initialized = 1, writing = 2, closing = 4};

    private:
    websocket::stream<beast::tcp_stream> &ws_;
    std::shared_ptr<session_client> p_s_;
    std::vector<std::string> v_message_;
    ws_write_state state_;

    public:
    ws_client_writer(std::shared_ptr<session_client> p_s
                    , websocket::stream<beast::tcp_stream> &ws):
                    ws_(ws)
                    , p_s_(p_s)
                    , state_(ws_write_state::none){

    }

    void
    send_frame(std::string fr){

        // std::cout << "send_frame ENTER >>>>" << std::endl;
        switch (state_){
            case ws_write_state::initialized:
                v_message_.push_back(fr);
                enter_write_state();
                break;
            case ws_write_state::writing:
            case ws_write_state::none:
                v_message_.push_back(fr);
                break;
            case ws_write_state::closing:
                // do nothing
                break;
        }
        // std::cout << "send_frame LEAVE >>>>" << std::endl;
    }

    void start(){
        state_ = ws_client_writer::initialized;
        
        if (v_message_.size() > 0)
            enter_write_state();
    }

    void close(){
        state_ = ws_client_writer::closing;

        ws_.async_close(websocket::close_code::normal,
            std::bind(
                    &ws_client_writer::on_close,
                    shared_from_this(),
                    std::placeholders::_1));

    }

    private:
    void
    enter_write_state(){
        // std::cout << "enter_write_state ENTER >>>>" << std::endl;

        ws_write_state cur_state = state_;
        switch(cur_state){
            case ws_write_state::initialized:
            {
                state_ = writing;
                std::string fr = v_message_.back();
                v_message_.pop_back();
                ws_.async_write(net::buffer(fr), std::bind(&ws_client_writer::on_write
                                                            , shared_from_this()
                                                            , std::placeholders::_1
                                                            , std::placeholders::_2));
            }
                break;
            case ws_write_state::writing:
            case ws_write_state::closing:
                std::cout << "do nothing" << std::endl;
                break;
        }

        // std::cout << "enter_write_state LEAVE <<<<" << std::endl;

    }

    void
    on_write(
    beast::error_code ec,
    std::size_t bytes_transferred){

        if (state_ == ws_client_writer::closing){
            return;
        }else {
            state_ = initialized;
            if (v_message_.size() > 0)
                return enter_write_state();
        }
    }

    void
    on_close(beast::error_code ec)
    {
        if(ec){
            std::cout << "on_close" << std::endl;
            return;
        }

    }
};


class ws_client_reader: public std::enable_shared_from_this<ws_client_reader>
{
    websocket::stream<beast::tcp_stream> &ws_;

    private:
    std::shared_ptr<session_client> p_s_client;
    beast::flat_buffer read_buffer_;

    private:
    void transition_to_close(){
    }

    void
    do_read();

    public:
    ws_client_reader(std::shared_ptr<session_client> s_p_
                    , websocket::stream<beast::tcp_stream> &ws);

    void
    on_read(beast::error_code ec, std::size_t bytes_transferred);

    void 
    start();
};


class session_client : public std::enable_shared_from_this<session_client>
{
    tcp::resolver resolver_;
    websocket::stream<beast::tcp_stream> ws_;
    std::string host_;

    std::shared_ptr<ws_client_reader> s_p_reader;
    std::shared_ptr<ws_client_writer> s_p_writer;


public:
    // Resolver and socket require an io_context
    explicit
    session_client(net::io_context& ioc)
        : resolver_(ioc)
        , ws_(ioc)
    {
    }

    void
    prepare(){
        s_p_reader = std::make_shared<ws_client_reader>(shared_from_this(), ws_);
        s_p_writer = std::make_shared<ws_client_writer>(shared_from_this(), ws_);
    }

    // Start the asynchronous operation
    void
    run(
        char const* host,
        char const* port)
    {
        // Save these for later
        host_ = host;

        // Look up the domain name
        resolver_.async_resolve(
            host,
            port,
            std::bind(
                &session_client::on_resolve,
                shared_from_this(),
                std::placeholders::_1,
                std::placeholders::_2));
    }

    void
    on_resolve(
        beast::error_code ec,
        tcp::resolver::results_type results)
    {
        if(ec)
            return fail(ec, "resolve");

        // Set the timeout for the operation
        beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

        // Make the connection on the IP address we get from a lookup
        beast::get_lowest_layer(ws_).async_connect(
            results,
            std::bind(
                &session_client::on_connect,
                shared_from_this(),
                std::placeholders::_1,
                std::placeholders::_2));
    }

    void
    on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep)
    {
        if(ec)
            return fail(ec, "connect");

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
                        " ws_client");
            }));

        // Update the host_ string. This will provide the value of the
        // Host HTTP header during the WebSocket handshake.
        // See https://tools.ietf.org/html/rfc7230#section-5.4
        host_ += ':' + std::to_string(ep.port());

        // Perform the websocket handshake
        ws_.async_handshake(host_, "/echo",
            std::bind(
                &session_client::on_handshake,
                shared_from_this(),
                std::placeholders::_1));
    }

    void
    on_handshake(beast::error_code ec)
    {
        // std::cout << "on_handshake ENTER >>>>" << std::endl;
        if(ec)
            return fail(ec, "handshake");
        
        // start reader
        s_p_reader->start();

        // start writer
        s_p_writer->start();

        // std::cout << "on_handshake LEAVE >>>>" << std::endl;

    }

    void
    fail(beast::error_code ec, char const* what)
    {
        std::cerr << what << ": " << ec.message() << "\n";
    }


    public:

    void send_frame(std::string fr){
        s_p_writer->send_frame(fr);
    }

    void
    close(){
        s_p_writer->close();
    }
};