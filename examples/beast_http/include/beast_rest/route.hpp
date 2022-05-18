#pragma once
#include <string>
#include <map>

#include <boost/beast/http/verb.hpp>
#include <boost/beast/core.hpp>

#include "beast_rest/session.hpp"

namespace beast_rest{

class route{

    public:

        route(){

        }
        route(const beast_rest::route &r){
            routes = std::move(r.routes);

        }

        void add(http::verb method, std::string reg, std::function<void(std::shared_ptr<beast_rest::request>)> &&func ){

            routes[reg] = std::move(func);

        }


        std::function<void(std::shared_ptr<beast_rest::request>)> match_handle(http::verb method, std::string uri){

            for (auto it = routes.begin(); it != routes.end(); it++){
                std::string reg = it->first;
                if (reg.compare(uri) == 0){
                    return it->second;
                }
            }
            
            return nullptr;
        }

        void dispatch_request(std::shared_ptr<beast_rest::request> req){
            
            // std::cout << "Handle request: Enter" << std::endl;
            for (auto it = routes.begin(); it != routes.end(); it++){
                std::string reg = it->first;
                    // std::cout << "Handle request DEBUG: " << reg << " target: " << req.req_ << std::endl;
                    auto path = req.get()->req_.target();
                    std::cout << "Handle request DEBUG: path " << path << std::endl;

                if (reg.compare(path) == 0){
                    // std::cout << "Handle request: " << reg << std::endl;
                    it->second(std::move(req));
                    return;
                }
            }

            req.get()->res_.result(http::status::bad_request);

            std::cout << "Handle request: Leave" << std::endl;

        }

    public:
        std::map<std::string, std::function<void(std::shared_ptr<beast_rest::request>) >> routes;
};

}
