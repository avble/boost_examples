
#include <beast_rest/app.hpp>

//------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    // Check command line arguments.
    if (argc != 3)
    {
        std::cerr <<
            "Example:\n" <<
            "    http-server-async 0.0.0.0 8080 . 1\n";
        return EXIT_FAILURE;
    }

    auto const address = net::ip::make_address(argv[1]);
    auto const port = static_cast<unsigned short>(std::atoi(argv[2]));

    // The io_context is required for all I/O
    net::io_context ioc;

    // route definitions
    beast_rest::route rs;
    
    auto h1 = [](std::shared_ptr<beast_rest::request> req){
        req.get()->res_.body() = "this is response from handle 01";
        req.get()->res_.prepare_payload();
    };

    rs.add(http::verb::get, "/hello", std::move(h1));

    // Create and launch a listening port
    auto app = std::make_shared<beast_rest::rest_app>(
        ioc,
        tcp::endpoint{address, port},
        rs);

    app->run();

    ioc.run();
    return EXIT_SUCCESS;
}