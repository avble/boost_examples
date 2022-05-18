#include <catch2/catch.hpp>

#include <iostream>
#include <chrono>

#include "beast_rest/config.hpp"

#include "beast_rest/session.hpp"
#include "beast_rest/route.hpp"

TEST_CASE("test_route_1"){

    beast_rest::route r;
    bool b_index1 = false;
    bool b_index2 = false;

    auto index1_handle = [&b_index1](std::shared_ptr<beast_rest::request> req){
        b_index1 = true;
    };

    auto index2_handle = [&b_index2](std::shared_ptr<beast_rest::request> req){
        b_index2 = true;
    };

    auto cancel_handle = [&b_index1, &b_index2](std::shared_ptr<beast_rest::request> req){
        b_index1 = false;
        b_index2 = false;
    };


    r.add(http::verb::get, "/index1", std::move(index1_handle));
    r.add(http::verb::get, "/index2", std::move(index2_handle));
    r.add(http::verb::get, "/cancel", std::move(cancel_handle));

    auto handle = r.match_handle(http::verb::get, "/index1");
    handle(std::make_shared<beast_rest::request>());
    REQUIRE(b_index1 == true);
    REQUIRE(b_index2 == false);

    handle = r.match_handle(http::verb::get, "/index2");
    handle(std::make_shared<beast_rest::request>());
    REQUIRE(b_index1 == true);
    REQUIRE(b_index2 == true);

    handle = r.match_handle(http::verb::get, "/cancel");
    handle(std::make_shared<beast_rest::request>());
    REQUIRE(b_index1 == false);
    REQUIRE(b_index2 == false);

}