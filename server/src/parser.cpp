#include "parser.h"
#include "catalog.h"
#include "storage.h"
#include "list.h"

#include <string>
#include <sstream>
#include <algorithm>
#include <cctype>

using namespace std;

static string trim(string s) {
    while (!s.empty() && isspace(s.front())) {
        s.erase(s.begin());
    }

    while (!s.empty() && isspace(s.back())) {
        s.pop_back();
    }

    return s;
}

static string upper(string s) {
    transform(s.begin(), s.end(), s.begin(), ::toupper);
    return s;
}

static bool startsWith(const string& text, const string& prefix) {
    return upper(text).rfind(prefix, 0) == 0;
}

static QueryResponse createDatabase(const string& query) {
    string name = trim(query.substr(15));

    if (!name.empty() && name.back() == ';') {
        name.pop_back();
    }

    name = trim(name);

    if (name.empty()) {
        return {false, "Error: nombre de base de datos vacío", ""};
    }

    if (databaseExists(name)) {
        return {false, "Error: la base de datos ya existe", ""};
    }

    bool folderCreated = createDatabaseFolder(name);

    if (!folderCreated && !databaseFolderExists(name)) {
        return {false, "Error: no se pudo crear la carpeta de la base de datos", ""};
    }

    registerDatabase(name);

    return {true, "Base de datos creada: " + name, ""};
}

static QueryResponse setDatabase(const string& query, string& currentDatabase) {
    string name = trim(query.substr(12));

    if (!name.empty() && name.back() == ';') {
        name.pop_back();
    }

    name = trim(name);

    if (name.empty()) {
        return {false, "Error: nombre de base de datos vacío", currentDatabase};
    }

    if (!databaseExists(name)) {
        return {false, "Error: la base de datos no existe", currentDatabase};
    }

    currentDatabase = name;

    return {true, "Base de datos actual: " + name, currentDatabase};
}

static QueryResponse createTable(const string& query, const string& currentDatabase) {
    if (currentDatabase.empty()) {
        return {false, "Error: primero debe ejecutar SET DATABASE", currentDatabase};
    }

    size_t nameStart = string("CREATE TABLE").size();
    size_t parenStart = query.find("(");
    size_t parenEnd = query.rfind(")");

    if (parenStart == string::npos || parenEnd == string::npos) {
        return {false, "Error: sintaxis inválida en CREATE TABLE", currentDatabase};
    }

    string tableName = trim(query.substr(nameStart, parenStart - nameStart));
    string columnsText = query.substr(parenStart + 1, parenEnd - parenStart - 1);

    if (tableName.empty()) {
        return {false, "Error: nombre de tabla vacío", currentDatabase};
    }

    if (tableExists(currentDatabase, tableName)) {
        return {false, "Error: la tabla ya existe", currentDatabase};
    }

    ColumnList columns;
    initColumnList(columns);

    size_t start = 0;

    while (start < columnsText.length()) {
        size_t comma = columnsText.find(",", start);
        string def;

        if (comma == string::npos) {
            def = columnsText.substr(start);
            start = columnsText.length();
        } else {
            def = columnsText.substr(start, comma - start);
            start = comma + 1;
        }

        def = trim(def);

        if (def.empty()) {
            continue;
        }

        size_t space = def.find(" ");

        if (space == string::npos) {
            freeColumnList(columns);
            return {false, "Error: columna inválida: " + def, currentDatabase};
        }

        string colName = trim(def.substr(0, space));
        string colType = trim(def.substr(space + 1));

        if (colName.empty() || colType.empty()) {
            freeColumnList(columns);
            return {false, "Error: columna inválida: " + def, currentDatabase};
        }

        addColumn(columns, colName, colType);
    }

    if (columns.head == nullptr) {
        freeColumnList(columns);
        return {false, "Error: la tabla debe tener al menos una columna", currentDatabase};
    }

    if (!createTableFile(currentDatabase, tableName)) {
        freeColumnList(columns);
        return {false, "Error: no se pudo crear archivo de tabla", currentDatabase};
    }

    registerTable(currentDatabase, tableName);
    registerColumns(currentDatabase, tableName, columns);

    freeColumnList(columns);

    return {true, "Tabla creada: " + tableName, currentDatabase};
}

static QueryResponse dropTable(const string& query, const string& currentDatabase) {
    if (currentDatabase.empty()) {
        return {false, "Error: primero debe ejecutar SET DATABASE", currentDatabase};
    }

    string tableName = trim(query.substr(10));

    if (!tableName.empty() && tableName.back() == ';') {
        tableName.pop_back();
    }

    tableName = trim(tableName);

    if (tableName.empty()) {
        return {false, "Error: nombre de tabla vacío", currentDatabase};
    }

    if (!tableExists(currentDatabase, tableName)) {
        return {false, "Error: la tabla no existe", currentDatabase};
    }

    if (!isTableEmpty(currentDatabase, tableName)) {
        return {false, "Error: solo se puede eliminar una tabla vacía", currentDatabase};
    }

    dropTableFile(currentDatabase, tableName);
    unregisterTable(currentDatabase, tableName);

    return {true, "Tabla eliminada: " + tableName, currentDatabase};
}

QueryResponse executeQuery(const string& rawQuery, string& currentDatabase) {
    string query = trim(rawQuery);

    if (query.empty()) {
        return {false, "Error: consulta vacía", currentDatabase};
    }

    if (startsWith(query, "CREATE DATABASE")) {
        return createDatabase(query);
    }

    if (startsWith(query, "SET DATABASE")) {
        return setDatabase(query, currentDatabase);
    }

    if (startsWith(query, "CREATE TABLE")) {
        return createTable(query, currentDatabase);
    }

    if (startsWith(query, "DROP TABLE")) {
        return dropTable(query, currentDatabase);
    }

    return {false, "Error: sentencia no soportada por Estudiante A", currentDatabase};
}