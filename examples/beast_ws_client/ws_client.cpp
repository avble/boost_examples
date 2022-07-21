#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/strand.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>

#include "boost/asio.hpp"

#include "ws_client.hpp"

ws_client_reader::ws_client_reader(std::shared_ptr<session_client> s_p_
                                    , websocket::stream<beast::tcp_stream> &ws):
    p_s_client(s_p_)
    , ws_(ws){

}

void ws_client_reader::start(){
    do_read();
}

void
ws_client_reader::do_read(){
    ws_.async_read(read_buffer_, std::bind(&ws_client_reader::on_read
                                           , shared_from_this()
                                           , std::placeholders::_1
                                           , std::placeholders::_2 ));
}

void
ws_client_reader::on_read(
    beast::error_code ec,
    std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);

    if(ec){
        transition_to_close();
        std::cout << "on_read: " << ec.message() << std::endl;
        return;
    }


    std::cout << beast::make_printable(read_buffer_.data()) << std::endl;

    read_buffer_.consume(bytes_transferred);
    do_read();
}
