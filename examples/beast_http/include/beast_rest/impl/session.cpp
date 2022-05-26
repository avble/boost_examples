#include "../app.hpp"
#include "../session.hpp"

namespace beast_rest{

void session::on_read(
    beast::error_code ec,
    std::size_t bytes_transferred){
    boost::ignore_unused(bytes_transferred);

    // This means they closed the connection
    if(ec == http::error::end_of_stream)
        return do_close();

    if(ec)
        return fail(ec, "read");

    std::shared_ptr<request> m_req = std::make_shared<request>(std::move(req_), send_res_);

    auto r = listener_.get()->get_route();
    r.dispatch_request(m_req);

    // continue reading another requests
    req_ = {};
    do_read();
};

};
    