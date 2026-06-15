#ifndef CATALOG_H
#define CATALOG_H

#include <string>
#include "list.h"

bool initCatalog();

bool registerDatabase(const std::string& dbName);
bool databaseExists(const std::string& dbName);

bool registerTable(const std::string& dbName, const std::string& tableName);
bool tableExists(const std::string& dbName, const std::string& tableName);
bool unregisterTable(const std::string& dbName, const std::string& tableName);

bool registerColumns(
    const std::string& dbName,
    const std::string& tableName,
    ColumnList& columns
);

#endif