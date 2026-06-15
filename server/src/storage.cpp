#include "storage.h"
#include <filesystem>
#include <fstream>

using namespace std;
namespace fs = std::filesystem;

bool initStorage() {
    fs::create_directories("databases");
    fs::create_directories("system_catalog");
    return true;
}

bool createDatabaseFolder(const string& dbName) {
    string path = "databases/" + dbName;
    if (fs::exists(path)) return false;
    return fs::create_directory(path);
}

bool databaseFolderExists(const string& dbName) {
    return fs::exists("databases/" + dbName);
}

bool createTableFile(const string& dbName, const string& tableName) {
    string path = "databases/" + dbName + "/" + tableName + ".bin";
    if (fs::exists(path)) return false;

    ofstream file(path, ios::binary);
    return file.good();
}

bool tableFileExists(const string& dbName, const string& tableName) {
    return fs::exists("databases/" + dbName + "/" + tableName + ".bin");
}

bool dropTableFile(const string& dbName, const string& tableName) {
    string path = "databases/" + dbName + "/" + tableName + ".bin";
    if (!fs::exists(path)) return false;
    return fs::remove(path);
}

bool isTableEmpty(const string& dbName, const string& tableName) {
    string path = "databases/" + dbName + "/" + tableName + ".bin";
    if (!fs::exists(path)) return false;
    return fs::file_size(path) == 0;
}