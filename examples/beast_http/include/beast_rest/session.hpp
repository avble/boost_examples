#pragma once
#include "config.hpp"

namespace beast_rest{
class listener;

class request{

    public:

        request():
        req_(http::verb::get, "index", 10),
        send_(nullptr),
        res_(http::status::ok,  req_.version()){

        }

        request(http::request<http::string_body>&& req):
        send_(nullptr),
        res_(http::status::ok,  req_.version()){

        }

        // request(http::request<http::string_body>&& req, send_res_func send):
        request(http::request<http::string_body>&& req, 
                    std::function<void(http::response<http::string_body> &&)> send):
        send_(send),
        req_(std::move(req)),
        res_(http::status::ok,  req_.version())
        {
            res_.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        }

        /**
         * send the response on the destructor
         */
        ~request(){
            std::cout << "[" << this << "]" << "~request 01" << std::endl;
            if (send_ != nullptr){
                std::cout << "~request 01.01" << std::endl;
                send_(std::move(res_));
                send_ = nullptr;
            }
            std::cout << "[" << this << "]" << "~request 02" << std::endl;
        }

    public:
        std::function<void(http::response<http::string_body>&& )> send_;
        http::request<http::string_body> req_;
        http::response<http::string_body> res_;

};

// template<
//     class Send>
// void
// handle_request(
//     http::request<http::string_body>&& req,
//     Send&& send)
// {

//     // select appropriate the rest service
//     request m_req(std::move(req), send);
//    // std::cout << "test 01" << std::endl;

//     request_handle_01(std::move(m_req));
//    // std::cout << "test 02" << std::endl;

// }

//------------------------------------------------------------------------------
// Report a failure
static void
fail(beast::error_code ec, char const* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

class session : public std::enable_shared_from_this<session>
{
    // This is the C++11 equivalent of a generic lambda.
    // The function object is used to send an HTTP message.
    struct send_res_lambda
    {
        session& self_;

        explicit
        send_res_lambda(session& self)
            : self_(self)
        {
        }

        template<class Body>
        void
        operator()(http::response<Body>&& msg) const
        {
            // The lifetime of the message has to extend
            // for the duration of the async operation so
            // we use a shared_ptr to manage it.
            auto sp = std::make_shared<
                http::response<Body>>(std::move(msg));

            self_.res_ = sp;

           // std::cout << "http-response: ENTER" << std::endl;
            // Write the response
            http::async_write(
                self_.stream_,
                *sp,
                beast::bind_front_handler(
                    &session::on_write,
                    self_.shared_from_this(),
                    sp->need_eof()));

           // std::cout << "http_response: LEAVE" << std::endl;
        }
    };

    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> req_;
    send_res_lambda send_res_;
    std::shared_ptr<void> res_;
    std::shared_ptr<listener> listener_;

public:
    // Take ownership of the stream
    session(
        tcp::socket&& socket,
        std::shared_ptr<listener> listen
        )
        : stream_(std::move(socket))
        , send_res_(*this)
        , listener_(listen)
    {
    }

    // Start the asynchronous operation
    void
    run()
    {
        net::dispatch(stream_.get_executor(),
                      beast::bind_front_handler(
                          &session::do_read,
                          shared_from_this()));
    }

    void
    do_read()
    {
        // Make the request empty before reading,
        // otherwise the operation behavior is undefined.
        req_ = {};

        // Set the timeout.
        stream_.expires_after(std::chrono::seconds(120));

        // Read a request
        http::async_read(stream_, buffer_, req_,
            beast::bind_front_handler(
                &session::on_read,
                shared_from_this()));
    }

    void
    on_read(
        beast::error_code ec,
        std::size_t bytes_transferred);
    // {
    //     boost::ignore_unused(bytes_transferred);

    //     // This means they closed the connection
    //     if(ec == http::error::end_of_stream)
    //         return do_close();

    //     if(ec)
    //         return fail(ec, "read");

    //     // dispatch to the corresponding service
    //     // handle request
    //     // handle_request(std::move(req_), send_res_);
    //     auto r = listener_.get();

    //     // r.handle_request(std::move(req_));

    //     // TODO: continue reading
    //     do_read();
    // }

    void
    on_write(
        bool close,
        beast::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

       // std::cout << "on_write: ENTER" << std::endl;
        if(ec)
            return fail(ec, "write");

        if(close)
        {
            // This means we should close the connection, usually because
            // the response indicated the "Connection: close" semantic.
           // std::cout << "on_write: do_close" << std::endl;
            return do_close();
        }
        
        res_ = nullptr;

       // std::cout << "on_write: LEAVE" << std::endl;

    }

    void
    do_close()
    {
        // Send a TCP shutdown
       // std::cout << "do_close: ENTER" << std::endl;
        beast::error_code ec;
        stream_.socket().shutdown(tcp::socket::shutdown_send, ec);

       // std::cout << "do_close: LEAVE" << std::endl;
        // At this point the connection is closed gracefully
    }
};

}