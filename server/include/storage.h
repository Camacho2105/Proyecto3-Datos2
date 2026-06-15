#ifndef STORAGE_H
#define STORAGE_H

#include <string>

bool initStorage();
bool createDatabaseFolder(const std::string& dbName);
bool databaseFolderExists(const std::string& dbName);
bool createTableFile(const std::string& dbName, const std::string& tableName);
bool tableFileExists(const std::string& dbName, const std::string& tableName);
bool dropTableFile(const std::string& dbName, const std::string& tableName);
bool isTableEmpty(const std::string& dbName, const std::string& tableName);

#endif