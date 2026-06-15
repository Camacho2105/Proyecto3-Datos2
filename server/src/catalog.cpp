#include "catalog.h"
#include <fstream>
#include <cstdio>

using namespace std;

static const string SYSTEM_DATABASES = "system_catalog/SystemDatabases.bin";
static const string SYSTEM_TABLES = "system_catalog/SystemTables.bin";
static const string SYSTEM_COLUMNS = "system_catalog/SystemColumns.bin";
static const string SYSTEM_INDEXES = "system_catalog/SystemIndexes.bin";

bool initCatalog() {
    ofstream(SYSTEM_DATABASES, ios::app | ios::binary).close();
    ofstream(SYSTEM_TABLES, ios::app | ios::binary).close();
    ofstream(SYSTEM_COLUMNS, ios::app | ios::binary).close();
    ofstream(SYSTEM_INDEXES, ios::app | ios::binary).close();
    return true;
}

bool databaseExists(const string& dbName) {
    ifstream file(SYSTEM_DATABASES, ios::binary);
    string line;

    while (getline(file, line)) {
        if (line == dbName) return true;
    }

    return false;
}

bool registerDatabase(const string& dbName) {
    if (databaseExists(dbName)) return false;

    ofstream file(SYSTEM_DATABASES, ios::app | ios::binary);
    file << dbName << "\n";
    return true;
}

bool tableExists(const string& dbName, const string& tableName) {
    ifstream file(SYSTEM_TABLES, ios::binary);
    string line;

    while (getline(file, line)) {
        if (line == dbName + "|" + tableName) return true;
    }

    return false;
}

bool registerTable(const string& dbName, const string& tableName) {
    if (tableExists(dbName, tableName)) return false;

    ofstream file(SYSTEM_TABLES, ios::app | ios::binary);
    file << dbName << "|" << tableName << "\n";
    return true;
}

bool registerColumns(
    const string& dbName,
    const string& tableName,
    ColumnList& columns
) {
    ofstream file(SYSTEM_COLUMNS, ios::app | ios::binary);

    ColumnNode* current = columns.head;

    while (current != nullptr) {
        file << dbName << "|"
             << tableName << "|"
             << current->data.name << "|"
             << current->data.type << "\n";

        current = current->next;
    }

    return true;
}

bool unregisterTable(const string& dbName, const string& tableName) {
    ifstream inTables(SYSTEM_TABLES, ios::binary);
    ofstream tempTables("system_catalog/temp_tables.bin", ios::binary);

    string line;

    while (getline(inTables, line)) {
        if (line != dbName + "|" + tableName) {
            tempTables << line << "\n";
        }
    }

    inTables.close();
    tempTables.close();

    remove(SYSTEM_TABLES.c_str());
    rename("system_catalog/temp_tables.bin", SYSTEM_TABLES.c_str());

    ifstream inColumns(SYSTEM_COLUMNS, ios::binary);
    ofstream tempColumns("system_catalog/temp_columns.bin", ios::binary);

    string prefix = dbName + "|" + tableName + "|";

    while (getline(inColumns, line)) {
        if (line.rfind(prefix, 0) != 0) {
            tempColumns << line << "\n";
        }
    }

    inColumns.close();
    tempColumns.close();

    remove(SYSTEM_COLUMNS.c_str());
    rename("system_catalog/temp_columns.bin", SYSTEM_COLUMNS.c_str());

    return true;
}