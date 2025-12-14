#include "api.hpp"
#include "pistache/endpoint.h"
#include "pistache/router.h"
#include "pistache/http_headers.h"
#include <nlohmann/json.hpp>

using namespace Pistache;
using namespace Pistache::Rest;
using json = nlohmann::json;

class RequestHandler: public Http::Handler {
    private:
        API& api;
        Router* router;

        void addCors(Http::ResponseWriter& response) {
            response.headers()
                .add<Http::Header::AccessControlAllowOrigin>("*")
                .add<Http::Header::AccessControlAllowMethods>("GET, PUT, POST, DELETE, OPTIONS")
                .add<Http::Header::AccessControlAllowHeaders>("Content-Type, Authorization, X-Requested-With, Accept");
        }

    public:
        RequestHandler(API& api, Router& router): api(api), router(&router) {
            Routes::Get(router, "/shopping_list/:id", Routes::bind(&RequestHandler::getList, this));
            Routes::Put(router, "/shopping_list", Routes::bind(&RequestHandler::createList, this));
            Routes::Delete(router, "/shopping_list/:id", Routes::bind(&RequestHandler::deleteList, this));
            Routes::Put(router, "/shopping_list/:id/item/:item_id", Routes::bind(&RequestHandler::updateItem, this));
            Routes::Post(router, "/shopping_list/:id/item", Routes::bind(&RequestHandler::addItem, this));
            Routes::Delete(router, "/shopping_list/:id/item/:item_id", Routes::bind(&RequestHandler::removeItem, this));

            Routes::Options(router, "/shopping_list", Routes::bind(&RequestHandler::optionsAny, this));
            Routes::Options(router, "/shopping_list/:id", Routes::bind(&RequestHandler::optionsAny, this));
            Routes::Options(router, "/shopping_list/:id/item", Routes::bind(&RequestHandler::optionsAny, this));
            Routes::Options(router, "/shopping_list/:id/item/:item_id", Routes::bind(&RequestHandler::optionsAny, this));
        }
        HTTP_PROTOTYPE(RequestHandler);

        void onRequest(const Http::Request& request, Http::ResponseWriter response) override {
            router->route(request, std::move(response));
        }

        void getList(const Rest::Request& request, Http::ResponseWriter response) {
            auto id = request.param(":id").as<std::string>();
            try {
                ShoppingList list = api.getShoppingList(id);
                addCors(response);
                response.send(Http::Code::Ok, to_json(list).dump(), MIME(Application, Json));
            } catch (const std::exception& e) {
                addCors(response);
                response.send(Http::Code::Not_Found, "List not found");
            }
        }

        void createList(const Rest::Request& request, Http::ResponseWriter response) {
            auto body = request.body();
            auto jsonBody = nlohmann::json::parse(body);
            std::string name = jsonBody["name"];
            ShoppingList list = api.createShoppingList(name);
            addCors(response);
            response.send(Http::Code::Ok, to_json(list).dump(), MIME(Application, Json));
        }

        void deleteList(const Rest::Request& request, Http::ResponseWriter response) {
            auto id = request.param(":id").as<std::string>();
            api.deleteShoppingList(id);
            addCors(response);
            response.send(Http::Code::Ok);
        }

        void updateItem(const Rest::Request& request, Http::ResponseWriter response) {
            auto listID = request.param(":id").as<std::string>();
            auto itemID = request.param(":item_id").as<std::string>();
            auto body = request.body();
            auto jsonBody = nlohmann::json::parse(body);
            std::string itemName = jsonBody["name"];
            int desiredQuantity = jsonBody["desired_quantity"];
            int currentQuantity = jsonBody["current_quantity"];
            ShoppingList list = api.updateItem(listID, itemID, itemName, desiredQuantity, currentQuantity);
            addCors(response);
            response.send(Http::Code::Ok, to_json(list).dump(), MIME(Application, Json));
        }

        void addItem(const Rest::Request& request, Http::ResponseWriter response) {
            auto listID = request.param(":id").as<std::string>();
            auto body = request.body();
            auto jsonBody = nlohmann::json::parse(body);
            std::string itemName = jsonBody["name"];
            int desiredQuantity = jsonBody["desired_quantity"];
            int currentQuantity = jsonBody["current_quantity"];
            ShoppingList list = api.addItem(listID, itemName, desiredQuantity, currentQuantity);
            addCors(response);
            response.send(Http::Code::Ok, to_json(list).dump(), MIME(Application, Json));
        }

        void removeItem(const Rest::Request& request, Http::ResponseWriter response) {
            auto listID = request.param(":id").as<std::string>();
            auto itemID = request.param(":item_id").as<std::string>();
            ShoppingList list = api.removeItem(listID, itemID);
            addCors(response);
            response.send(Http::Code::Ok, to_json(list).dump(), MIME(Application, Json));
        }

        void optionsAny(const Rest::Request&, Http::ResponseWriter response) {
            addCors(response);
            response.send(Http::Code::No_Content);
        }
};