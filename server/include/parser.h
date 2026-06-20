#ifndef PARSER_H
#define PARSER_H

#include <string>
#include "json.hpp"

struct QueryResponse {
    bool success;
    std::string message;
    std::string currentDatabase;
    nlohmann::json data;
};

QueryResponse executeQuery(
    const std::string& query,
    std::string& currentDatabase
);

#endif