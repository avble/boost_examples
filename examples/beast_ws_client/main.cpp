#include <thread>

#include <variant>
#include <string_view>

#include "config.hpp"
#include "ws_client.hpp"

int 
main(int argc, char** argv)
{
    // Check command line arguments.
    if(argc < 3)
    {
        std::cerr <<
            "Usage: ws_client <host> <port>\n" <<
            "Example:\n" <<
            "    ws_client 127.0.0.1 80 \n";
        return EXIT_FAILURE;
    }
    auto const host = argv[1];
    auto const port = argv[2];
    auto target = "";
    if (argc == 4)
        target = argv[3];

    // The io_context is required for all I/O
    net::io_context ioc;

    std::shared_ptr<session_concept> ws_client;

    if (port != "443") 
        ws_client = std::shared_ptr<session_concept>(new session_plain(ioc));
    else
        ws_client = std::shared_ptr<session_concept>(new session_ssl(ioc));
    
    ws_client->run(host, port, target,
                    [](boost::system::error_code ec, std::string_view read_data){
                        std::cout << "on_read data: \n" << std::string(read_data.data(), read_data.size()) << std::endl;
                    }
    );

    // Run the I/O service. The call will return when
    auto thread_io = [&ioc](){
        ioc.run();
    };
    std::thread thr_(thread_io);

    bool running = true;
    do{
        std::string line;
        std::getline(std::cin, line);
        if (line == "exit"){
            ws_client->close();
            running = false;
        }else if (line != ""){
            ws_client->send_frame(line);
        }
    }while(running);

    std::cout << "end!" << std::endl;
    thr_.join();
    std::cout << "end end!" << std::endl;
    return EXIT_SUCCESS;
}