#ifndef PARSER_H
#define PARSER_H

#include <string>

struct QueryResponse {
    bool success;
    std::string message;
    std::string currentDatabase;
};

QueryResponse executeQuery(
    const std::string& query,
    std::string& currentDatabase
);

#endif