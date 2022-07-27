#pragma once

#include "config.hpp"
#include "boost/beast/websocket/ssl.hpp"
#include "boost/beast/ssl.hpp"

template <typename ws_client>
class ws_client_writer: public std::enable_shared_from_this<ws_client_writer<ws_client>>{

    enum ws_write_state {none = 0, initialized = 1, writing = 2, closing = 4};

    private:
    ws_client &ws_client_;
    std::vector<std::string> v_message_;
    ws_write_state state_;

    public:
    ws_client_writer(ws_client &ws_c_):
                    ws_client_(ws_c_)
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

        ws_client_.get_impl().get_ws().async_close(websocket::close_code::normal,
            std::bind(
                    &ws_client_writer::on_close,
                    this->shared_from_this(),
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
                ws_client_.get_impl().get_ws().async_write(net::buffer(fr), std::bind(&ws_client_writer::on_write
                                                            , this->shared_from_this()
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


class session_concept{

    public:
    virtual void
    run(
        char const* host,
        char const* port,
        char const* target) = 0;

    virtual void
    send_frame(std::string) = 0;

    virtual void
    close() = 0;

};

template <typename Derived>
class session_client: public session_concept
{
    typedef ws_client_writer<session_client<Derived>> writer_t;

    std::string host_;
    std::string target_;
    tcp::resolver resolver_;

    std::shared_ptr<writer_t> writer_;

    beast::flat_buffer read_buffer_;

    public:
    explicit
    session_client(net::io_context& ioc)
        : resolver_(ioc){
        
        writer_ = std::make_shared<writer_t>(*this);
        target_ = "";
    }

    Derived &
    get_impl(){
        return static_cast<Derived&>(*this);
    }

    // Start the asynchronous operation
    void
    run(
        char const* host,
        char const* port,
        char const* target)
    {
        host_ = host;
        target_ = target;

        auto self = get_impl().shared_from_this();

        resolver_.async_resolve(
            host,
            port,
            std::bind(
                &session_client::on_resolve,
                self,
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
        beast::get_lowest_layer(get_impl().get_ws()).expires_after(std::chrono::seconds(30));

        // Make the connection on the IP address we get from a lookup
        beast::get_lowest_layer(get_impl().get_ws()).async_connect(
            results,
            std::bind(
                &session_client::on_connect,
                get_impl().shared_from_this(),
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
        beast::get_lowest_layer(get_impl().get_ws()).expires_never();

        // Set suggested timeout settings for the websocket
        get_impl().get_ws().set_option(
            websocket::stream_base::timeout::suggested(
                beast::role_type::client));

        // Set a decorator to change the User-Agent of the handshake
        get_impl().get_ws().set_option(websocket::stream_base::decorator(
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

        get_impl().prepare();

    }

    void
    do_handshake(){
        // Perform the websocket handshake
        get_impl().get_ws().async_handshake(host_, target_,
            std::bind(
                &session_client::on_handshake,
                get_impl().shared_from_this(),
                std::placeholders::_1));
    }

    void
    on_handshake(beast::error_code ec)
    {
        // std::cout << "on_handshake ENTER >>>>" << std::endl;
        if(ec)
            return fail(ec, "handshake");

        // start reading
        do_read();

        // start writing
        writer_->start();
    }

    void
    do_read(){
        get_impl().get_ws().async_read(read_buffer_, std::bind(&session_client::on_read
                                           , get_impl().shared_from_this()
                                           , std::placeholders::_1
                                           , std::placeholders::_2 ));
    }

    void
    on_read(
    beast::error_code ec,
    std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if(ec){
            std::cout << "on_read: " << ec.message() << std::endl;
            return;
        }

        std::cout << beast::make_printable(read_buffer_.data()) << std::endl;

        read_buffer_.consume(bytes_transferred);
        do_read();
    }

    void
    fail(beast::error_code ec, char const* what)
    {
        std::cerr << what << ": " << ec.message() << "\n";
    }

    public:
    void send_frame(std::string fr){
        writer_->send_frame(fr);
    }

    void
    close(){
        writer_->close();
    }
};

class session_plain: public session_client<session_plain>
                    , public std::enable_shared_from_this<session_plain>{

    typedef session_client<session_plain> base;
    typedef websocket::stream<beast::tcp_stream> ws_stream;

    public:
    session_plain(boost::asio::io_context &ioc_): base(ioc_)
                                                , ws_(ioc_){

    }

    ws_stream &
    get_ws(){
        return ws_;
    }

    void
    prepare(){
        base::do_handshake();
    }


    private:
    ws_stream ws_;

};
class session_ssl: public session_client<session_ssl>, 
                    public std::enable_shared_from_this<session_ssl>{

    typedef session_client<session_ssl> base;
    typedef websocket::stream<beast::ssl_stream<beast::tcp_stream>> ssl_websocket;

    public:
    session_ssl(boost::asio::io_context &ioc_): base(ioc_)
                                                , ssl_ctx_(boost::asio::ssl::context::tlsv12_client)
                                                , ws_(ioc_, ssl_ctx_){

    }

    ssl_websocket &
    get_ws(){
        return ws_;
    }

    void
    prepare(){

        ws_.next_layer().async_handshake(
            boost::asio::ssl::stream_base::client
            , std::bind( &session_ssl::on_ssl_handshake
                        , shared_from_this()
                        , std::placeholders::_1)
        );
    }

    void
    on_ssl_handshake(beast::error_code ec)
    {
        if(ec)
            return fail(ec, "ssl_handshake");

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

        base::do_handshake();

    }


    private:
    boost::asio::ssl::context ssl_ctx_;
    ssl_websocket ws_;

};