#define _WIN32_WINNT 0x0A00

#include "parser.h"
#include "catalog.h"
#include "storage.h"

#include "httplib.h"
#include "json.hpp"

#include <iostream>
#include <chrono>

using namespace std;
using json = nlohmann::json;

int main() {
    initStorage();
    initCatalog();

    string currentDatabase = "";

    httplib::Server server;

    server.Options("/query", [](const httplib::Request&, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        res.set_header("Access-Control-Allow-Methods", "POST, OPTIONS");
        res.status = 200;
    });

    server.Post("/query", [&](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");

        auto start = chrono::high_resolution_clock::now();

        json response;

        try {
            json body = json::parse(req.body);

            if (!body.contains("query")) {
                response["success"] = false;
                response["message"] = "Error: falta el campo query";
                response["currentDatabase"] = currentDatabase;
                response["executionTimeMs"] = 0;

                res.set_content(response.dump(4), "application/json");
                return;
            }

            string query = body["query"];

            QueryResponse result = executeQuery(query, currentDatabase);

            auto end = chrono::high_resolution_clock::now();
            auto ms = chrono::duration_cast<chrono::milliseconds>(end - start).count();

            response["success"] = result.success;
            response["message"] = result.message;
            response["currentDatabase"] = currentDatabase;
            response["executionTimeMs"] = ms;

        } catch (...) {
            response["success"] = false;
            response["message"] = "Error: JSON invalido";
            response["currentDatabase"] = currentDatabase;
            response["executionTimeMs"] = 0;
        }

        res.set_content(response.dump(4), "application/json");
    });

    cout << "Servidor TinySQLDb corriendo en http://localhost:8080" << endl;
    cout << "Endpoint disponible: POST http://localhost:8080/query" << endl;

    server.listen("0.0.0.0", 8080);

    return 0;
}