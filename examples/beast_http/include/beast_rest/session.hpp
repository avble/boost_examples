#pragma once

#include "config.hpp"

namespace beast_rest{
class request{

    public:
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
            send_(std::move(res_));
        }

    public:
        std::function<void(http::response<http::string_body>&& )> send_;
        http::request<http::string_body> req_;
        http::response<http::string_body> res_;

};

void request_handle_01(request &&rq){

    rq.res_.body() = "The resource is reponsed.";
    rq.res_.prepare_payload();
}


template<
    class Send>
void
handle_request(
    http::request<http::string_body>&& req,
    Send&& send)
{

    // select appropriate the rest service
    request m_req(std::move(req), send);

    request_handle_01(std::move(m_req));

}

//------------------------------------------------------------------------------
// Report a failure
void
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

            // Write the response
            http::async_write(
                self_.stream_,
                *sp,
                beast::bind_front_handler(
                    &session::on_write,
                    self_.shared_from_this(),
                    sp->need_eof()));
        }
    };

    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> req_;
    send_res_lambda send_res_;

    std::map<std::string, std::function<void(request &&)>> table_services;

public:
    // Take ownership of the stream
    session(
        tcp::socket&& socket)
        : stream_(std::move(socket))
        , send_res_(*this)
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
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        // This means they closed the connection
        if(ec == http::error::end_of_stream)
            return do_close();

        if(ec)
            return fail(ec, "read");

        // dispatch to the corresponding service
        // handle request
        handle_request(std::move(req_), send_res_);

        // TODO: continue reading
        do_read();
    }

    void
    on_write(
        bool close,
        beast::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if(ec)
            return fail(ec, "write");

        if(close)
        {
            // This means we should close the connection, usually because
            // the response indicated the "Connection: close" semantic.
            return do_close();
        }

    }

    void
    do_close()
    {
        // Send a TCP shutdown
        beast::error_code ec;
        stream_.socket().shutdown(tcp::socket::shutdown_send, ec);

        // At this point the connection is closed gracefully
    }
};

}