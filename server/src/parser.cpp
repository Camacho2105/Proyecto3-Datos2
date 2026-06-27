#include "../include/parser.h"
#include "../include/catalog.h"
#include "../include/storage.h"
#include "../include/list.h"
#include "../include/bst.h"
#include "../include/quicksort.h"
#include "../include/btree.h"

#include <string>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <vector>
#include <cstdio>

using namespace std;

static string trim(string s) {
    while (!s.empty() && isspace((unsigned char)s.front())) {
        s.erase(s.begin());
    }

    while (!s.empty() && isspace((unsigned char)s.back())) {
        s.pop_back();
    }

    return s;
}

static string upper(string s) {
    transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return toupper(c);
    });
    return s;
}

static bool startsWith(const string& text, const string& prefix) {
    return upper(text).rfind(prefix, 0) == 0;
}

static string removeSemicolon(string s) {
    s = trim(s);
    if (!s.empty() && s.back() == ';') {
        s.pop_back();
    }
    return trim(s);
}

static string stripQuotes(string s) {
    s = trim(s);
    if (s.size() >= 2) {
        if ((s.front() == '\'' && s.back() == '\'') ||
            (s.front() == '"' && s.back() == '"')) {
            return s.substr(1, s.size() - 2);
        }
    }
    return s;
}

static vector<string> splitCSV(const string& text) {
    vector<string> result;
    string current;
    bool inSingle = false;
    bool inDouble = false;

    for (char c : text) {
        if (c == '\'' && !inDouble) {
            inSingle = !inSingle;
            current += c;
        } else if (c == '"' && !inSingle) {
            inDouble = !inDouble;
            current += c;
        } else if (c == ',' && !inSingle && !inDouble) {
            result.push_back(trim(current));
            current.clear();
        } else {
            current += c;
        }
    }

    if (!current.empty() || !text.empty()) {
        result.push_back(trim(current));
    }

    return result;
}

static bool isValidInteger(const string& val) {
    string v = stripQuotes(val);
    if (v.empty()) return false;

    size_t start = 0;
    if (v[0] == '-') start = 1;
    if (start == v.length()) return false;

    for (size_t i = start; i < v.length(); i++) {
        if (!isdigit((unsigned char)v[i])) return false;
    }

    return true;
}

static bool isValidDouble(const string& val) {
    string v = stripQuotes(val);
    if (v.empty()) return false;

    size_t start = 0;
    if (v[0] == '-') start = 1;
    if (start == v.length()) return false;

    bool hasDot = false;

    for (size_t i = start; i < v.length(); i++) {
        if (v[i] == '.') {
            if (hasDot) return false;
            hasDot = true;
        } else if (!isdigit((unsigned char)v[i])) {
            return false;
        }
    }

    return true;
}

static bool isQuotedText(const string& val) {
    string v = trim(val);
    if (v.size() < 2) return false;

    return (v.front() == '\'' && v.back() == '\'') ||
           (v.front() == '"' && v.back() == '"');
}

static bool isValidTypeDefinition(const string& type) {
    string t = upper(trim(type));

    if (t == "INTEGER") return true;
    if (t == "DOUBLE") return true;
    if (t == "DATETIME") return true;

    if (t.rfind("VARCHAR(", 0) == 0 && t.back() == ')') {
        string number = t.substr(8, t.size() - 9);
        if (number.empty()) return false;

        for (char c : number) {
            if (!isdigit((unsigned char)c)) return false;
        }

        return stoi(number) > 0;
    }

    return false;
}

static bool validateValueForType(const string& value, const string& type, string& error) {
    string t = upper(trim(type));
    string v = trim(value);

    if (t == "INTEGER") {
        if (!isValidInteger(v)) {
            error = "'" + v + "' no es un INTEGER válido.";
            return false;
        }
        return true;
    }

    if (t == "DOUBLE") {
        if (!isValidDouble(v)) {
            error = "'" + v + "' no es un DOUBLE válido.";
            return false;
        }
        return true;
    }

    if (t.rfind("VARCHAR(", 0) == 0) {
        if (!isQuotedText(v)) {
            error = "'" + v + "' debe ir entre comillas simples o dobles.";
            return false;
        }

        string number = t.substr(8, t.size() - 9);
        int maxLen = stoi(number);
        string clean = stripQuotes(v);

        if ((int)clean.size() > maxLen) {
            error = "'" + clean + "' excede el tamaño máximo VARCHAR(" + to_string(maxLen) + ").";
            return false;
        }

        return true;
    }

    if (t == "DATETIME") {
        if (!isQuotedText(v)) {
            error = "'" + v + "' debe ir entre comillas para DATETIME.";
            return false;
        }

        return true;
    }

    error = "tipo de dato no soportado: " + type;
    return false;
}

static string cifrarDatos(const string& data) {
    string result = data;
    for (size_t i = 0; i < data.size(); ++i) {
        result[i] = result[i] + 3;
    }
    return result;
}

static string descifrarDatos(const string& data) {
    string result = data;
    for (size_t i = 0; i < data.size(); ++i) {
        result[i] = result[i] - 3;
    }
    return result;
}

static bool loadColumns(
    const string& currentDatabase,
    const string& tableName,
    vector<string>& colNames,
    vector<string>& colTypes
) {
    ifstream catFile("system_catalog/SystemColumns.bin", ios::binary);
    string line;
    string prefix = currentDatabase + "|" + tableName + "|";

    while (getline(catFile, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();

        if (line.rfind(prefix, 0) == 0) {
            string rest = line.substr(prefix.length());
            size_t pipe = rest.find('|');

            if (pipe != string::npos) {
                string colName = rest.substr(0, pipe);
                string colType = rest.substr(pipe + 1);

                colNames.push_back(trim(colName));
                colTypes.push_back(trim(colType));
            }
        }
    }

    catFile.close();
    return !colNames.empty();
}

static int findColumnIndex(const vector<string>& cols, const string& name) {
    for (int i = 0; i < (int)cols.size(); i++) {
        if (cols[i] == name) return i;
    }
    return -1;
}

static vector<string> parseRecordValues(const string& decryptedLine) {
    return splitCSV(decryptedLine);
}

static string normalizeStoredValue(string v) {
    v = trim(v);
    return stripQuotes(v);
}

struct Condition {
    bool active = false;
    string column;
    string op;
    string value;
};

static Condition parseCondition(string condition) {
    Condition c;
    condition = removeSemicolon(condition);

    if (condition.empty()) return c;

    string up = upper(condition);

    size_t pos = up.find(" LIKE ");
    if (pos != string::npos) {
        c.active = true;
        c.column = trim(condition.substr(0, pos));
        c.op = "LIKE";
        c.value = stripQuotes(trim(condition.substr(pos + 6)));
        return c;
    }

    pos = up.find(" NOT ");
    if (pos != string::npos) {
        c.active = true;
        c.column = trim(condition.substr(0, pos));
        c.op = "NOT";
        c.value = stripQuotes(trim(condition.substr(pos + 5)));
        return c;
    }

    pos = condition.find("==");
    if (pos != string::npos) {
        c.active = true;
        c.column = trim(condition.substr(0, pos));
        c.op = "=";
        c.value = stripQuotes(trim(condition.substr(pos + 2)));
        return c;
    }

    pos = condition.find('=');
    if (pos != string::npos) {
        c.active = true;
        c.column = trim(condition.substr(0, pos));
        c.op = "=";
        c.value = stripQuotes(trim(condition.substr(pos + 1)));
        return c;
    }

    pos = condition.find('>');
    if (pos != string::npos) {
        c.active = true;
        c.column = trim(condition.substr(0, pos));
        c.op = ">";
        c.value = stripQuotes(trim(condition.substr(pos + 1)));
        return c;
    }

    pos = condition.find('<');
    if (pos != string::npos) {
        c.active = true;
        c.column = trim(condition.substr(0, pos));
        c.op = "<";
        c.value = stripQuotes(trim(condition.substr(pos + 1)));
        return c;
    }

    return c;
}

static bool isNumber(const string& s) {
    return isValidInteger(s) || isValidDouble(s);
}

static bool compareValues(string left, string op, string right) {
    left = normalizeStoredValue(left);
    right = stripQuotes(trim(right));

    if (op == "=") return left == right;
    if (op == "NOT") return left != right;

    if (op == "LIKE") {
        string pattern = right;

        pattern.erase(remove(pattern.begin(), pattern.end(), '*'), pattern.end());
        pattern.erase(remove(pattern.begin(), pattern.end(), '%'), pattern.end());

        return left.find(pattern) != string::npos;
    }

    if (op == ">" || op == "<") {
        if (isNumber(left) && isNumber(right)) {
            double a = stod(left);
            double b = stod(right);

            if (op == ">") return a > b;
            if (op == "<") return a < b;
        }

        if (op == ">") return left > right;
        if (op == "<") return left < right;
    }

    return false;
}

static void rebuildIndex(
    const string& currentDatabase,
    const string& tableName,
    const string& indexName,
    const string& colName,
    const string& indexType
) {
    vector<string> colNames;
    vector<string> colTypes;
    loadColumns(currentDatabase, tableName, colNames, colTypes);

    int targetColIndex = findColumnIndex(colNames, colName);
    if (targetColIndex == -1) return;

    string path = "databases/" + currentDatabase + "/" + tableName + ".bin";
    ifstream file(path, ios::binary);

    IndexRecord records[1000];
    int recordCount = 0;
    int lineNumber = 1;
    string line;

    if (file.is_open()) {
        while (getline(file, line) && recordCount < 1000) {
            if (line.empty()) {
                lineNumber++;
                continue;
            }

            string decryptedLine = descifrarDatos(line);
            vector<string> values = parseRecordValues(decryptedLine);

            if (targetColIndex < (int)values.size()) {
                records[recordCount].key = normalizeStoredValue(values[targetColIndex]);
                records[recordCount].recordLine = lineNumber;
                recordCount++;
            }

            lineNumber++;
        }

        file.close();
    }

    if (recordCount > 0) {
        quickSort(records, 0, recordCount - 1);
    }

    if (indexType == "BST") {
        BST bstIndex;
        for (int i = 0; i < recordCount; ++i) {
            bstIndex.insert(records[i].key, records[i].recordLine);
        }
    } else {
        BTree btreeIndex(3);
        for (int i = 0; i < recordCount; ++i) {
            btreeIndex.insert(records[i].key, records[i].recordLine);
        }
    }

    string indexPath = "databases/" + currentDatabase + "/" + indexName + ".bin";
    ofstream idxFile(indexPath, ios::binary);

    if (idxFile.is_open()) {
        idxFile << "INDICE|" << indexType << "|" << colName << "\n";

        for (int i = 0; i < recordCount; ++i) {
            idxFile << records[i].key << "|" << records[i].recordLine << "\n";
        }

        idxFile.close();
    }
}

static void updateAllIndexesForTable(const string& currentDatabase, const string& tableName) {
    ifstream sysIdx("system_catalog/SystemIndexes.bin", ios::binary);
    if (!sysIdx.is_open()) return;

    string line;
    string prefix = currentDatabase + "|" + tableName + "|";

    struct IdxData {
        string name;
        string col;
        string type;
    };

    vector<IdxData> indexes;

    while (getline(sysIdx, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();

        if (line.rfind(prefix, 0) == 0) {
            vector<string> parts = splitCSV("");
            size_t p1 = line.find('|');
            size_t p2 = line.find('|', p1 + 1);
            size_t p3 = line.find('|', p2 + 1);
            size_t p4 = line.find('|', p3 + 1);

            if (p1 != string::npos && p2 != string::npos &&
                p3 != string::npos && p4 != string::npos) {
                IdxData idx;
                idx.name = line.substr(p2 + 1, p3 - p2 - 1);
                idx.col = line.substr(p3 + 1, p4 - p3 - 1);
                idx.type = line.substr(p4 + 1);
                indexes.push_back(idx);
            }
        }
    }

    sysIdx.close();

    for (const auto& idx : indexes) {
        rebuildIndex(currentDatabase, tableName, idx.name, idx.col, idx.type);
    }
}

static bool indexedValueExists(
    const string& currentDatabase,
    const string& tableName,
    const string& colName,
    const string& value
) {
    vector<string> colNames;
    vector<string> colTypes;
    loadColumns(currentDatabase, tableName, colNames, colTypes);

    int colIndex = findColumnIndex(colNames, colName);
    if (colIndex == -1) return false;

    string path = "databases/" + currentDatabase + "/" + tableName + ".bin";
    ifstream file(path, ios::binary);
    string line;
    string cleanValue = normalizeStoredValue(value);

    while (getline(file, line)) {
        if (line.empty()) continue;

        string decryptedLine = descifrarDatos(line);
        vector<string> values = parseRecordValues(decryptedLine);

        if (colIndex < (int)values.size()) {
            if (normalizeStoredValue(values[colIndex]) == cleanValue) {
                file.close();
                return true;
            }
        }
    }

    file.close();
    return false;
}

static bool checkDuplicateForIndexes(
    const string& currentDatabase,
    const string& tableName,
    const vector<string>& values,
    string& error
) {
    vector<string> colNames;
    vector<string> colTypes;
    loadColumns(currentDatabase, tableName, colNames, colTypes);

    ifstream sysIdx("system_catalog/SystemIndexes.bin", ios::binary);
    if (!sysIdx.is_open()) return true;

    string line;
    string prefix = currentDatabase + "|" + tableName + "|";

    while (getline(sysIdx, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();

        if (line.rfind(prefix, 0) == 0) {
            size_t p1 = line.find('|');
            size_t p2 = line.find('|', p1 + 1);
            size_t p3 = line.find('|', p2 + 1);
            size_t p4 = line.find('|', p3 + 1);

            if (p1 == string::npos || p2 == string::npos ||
                p3 == string::npos || p4 == string::npos) {
                continue;
            }

            string colName = line.substr(p3 + 1, p4 - p3 - 1);
            int colIndex = findColumnIndex(colNames, colName);

            if (colIndex != -1 && colIndex < (int)values.size()) {
                if (indexedValueExists(currentDatabase, tableName, colName, values[colIndex])) {
                    error = "Error: valor duplicado en columna indexada '" + colName + "': " + normalizeStoredValue(values[colIndex]);
                    sysIdx.close();
                    return false;
                }
            }
        }
    }

    sysIdx.close();
    return true;
}

static QueryResponse createDatabase(const string& query) {
    string name = removeSemicolon(query.substr(15));

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
    string name = removeSemicolon(query.substr(12));

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

    if (parenStart == string::npos || parenEnd == string::npos || parenStart >= parenEnd) {
        return {false, "Error: sintaxis inválida en CREATE TABLE", currentDatabase};
    }

    string tableName = trim(query.substr(nameStart, parenStart - nameStart));

    if (upper(tableName).size() >= 2 &&
        upper(tableName).substr(upper(tableName).size() - 2) == "AS") {
        tableName = trim(tableName.substr(0, tableName.size() - 2));
    }

    string columnsText = query.substr(parenStart + 1, parenEnd - parenStart - 1);

    if (tableName.empty()) {
        return {false, "Error: nombre de tabla vacío", currentDatabase};
    }

    if (tableExists(currentDatabase, tableName)) {
        return {false, "Error: la tabla ya existe", currentDatabase};
    }

    ColumnList columns;
    initColumnList(columns);

    vector<string> definitions = splitCSV(columnsText);

    for (string def : definitions) {
        def = trim(def);

        if (def.empty()) continue;

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

        if (!isValidTypeDefinition(colType)) {
            freeColumnList(columns);
            return {false, "Error: tipo de dato inválido: " + colType, currentDatabase};
        }

        addColumn(columns, colName, upper(colType));
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

static bool tableHasRows(const string& currentDatabase, const string& tableName) {
    string path = "databases/" + currentDatabase + "/" + tableName + ".bin";
    ifstream file(path, ios::binary);

    string line;
    while (getline(file, line)) {
        if (!trim(line).empty()) {
            file.close();
            return true;
        }
    }

    file.close();
    return false;
}

static QueryResponse dropTable(const string& query, const string& currentDatabase) {
    if (currentDatabase.empty()) {
        return {false, "Error: primero debe ejecutar SET DATABASE", currentDatabase};
    }

    string tableName = removeSemicolon(query.substr(10));

    if (tableName.empty()) {
        return {false, "Error: nombre de tabla vacío", currentDatabase};
    }

    if (!tableExists(currentDatabase, tableName)) {
        return {false, "Error: la tabla no existe", currentDatabase};
    }

    if (tableHasRows(currentDatabase, tableName)) {
        return {false, "Error: solo se puede eliminar una tabla vacía", currentDatabase};
    }

    dropTableFile(currentDatabase, tableName);
    unregisterTable(currentDatabase, tableName);

    return {true, "Tabla eliminada: " + tableName, currentDatabase};
}

static QueryResponse insertIntoTable(const string& query, const string& currentDatabase) {
    if (currentDatabase.empty()) {
        return {false, "Error: primero debe ejecutar SET DATABASE", currentDatabase};
    }

    string upperQuery = upper(query);

    size_t nameStart = string("INSERT INTO").size();
    size_t valuesPos = upperQuery.find(" VALUES");

    if (valuesPos == string::npos) {
        return {false, "Error: sintaxis inválida, falta VALUES", currentDatabase};
    }

    string tableName = trim(query.substr(nameStart, valuesPos - nameStart));

    size_t parenStart = query.find("(", valuesPos);
    size_t parenEnd = query.rfind(")");

    if (parenStart == string::npos || parenEnd == string::npos || parenStart >= parenEnd) {
        return {false, "Error: sintaxis inválida, faltan paréntesis en VALUES", currentDatabase};
    }

    if (tableName.empty()) {
        return {false, "Error: nombre de tabla vacío", currentDatabase};
    }

    if (!tableExists(currentDatabase, tableName)) {
        return {false, "Error: la tabla no existe en la base de datos", currentDatabase};
    }

    string valuesText = query.substr(parenStart + 1, parenEnd - parenStart - 1);
    vector<string> values = splitCSV(valuesText);

    vector<string> colNames;
    vector<string> colTypes;
    loadColumns(currentDatabase, tableName, colNames, colTypes);

    if (values.size() != colNames.size()) {
        return {
            false,
            "Error de columnas: La tabla espera " + to_string(colNames.size()) +
            " valores, pero se enviaron " + to_string(values.size()),
            currentDatabase
        };
    }

    for (int i = 0; i < (int)values.size(); i++) {
        string error;
        if (!validateValueForType(values[i], colTypes[i], error)) {
            return {
                false,
                "Error de tipo en columna '" + colNames[i] + "': " + error,
                currentDatabase
            };
        }
    }

    string duplicateError;
    if (!checkDuplicateForIndexes(currentDatabase, tableName, values, duplicateError)) {
        return {false, duplicateError, currentDatabase};
    }

    string normalizedLine = trim(values[0]);
    for (int i = 1; i < (int)values.size(); i++) {
        normalizedLine += ", " + trim(values[i]);
    }

    string path = "databases/" + currentDatabase + "/" + tableName + ".bin";
    ofstream file(path, ios::app | ios::binary);

    if (!file.is_open()) {
        return {false, "Error: no se pudo abrir el archivo para insertar datos", currentDatabase};
    }

    file << cifrarDatos(normalizedLine) << '\n';
    file.close();

    updateAllIndexesForTable(currentDatabase, tableName);

    return {true, "Fila insertada exitosamente en la tabla " + tableName, currentDatabase};
}

static QueryResponse selectFromTable(const string& query, const string& currentDatabase) {
    if (currentDatabase.empty()) {
        return {false, "Error: primero debe ejecutar SET DATABASE", currentDatabase};
    }

    string cleanQuery = removeSemicolon(query);
    string upperQuery = upper(cleanQuery);

    size_t selectPos = upperQuery.find("SELECT ");
    size_t fromPos = upperQuery.find(" FROM ");

    if (selectPos == string::npos || fromPos == string::npos) {
        return {false, "Error: sintaxis inválida, falta SELECT o FROM", currentDatabase};
    }

    string selectedText = trim(cleanQuery.substr(selectPos + 7, fromPos - (selectPos + 7)));

    size_t wherePos = upperQuery.find(" WHERE ", fromPos);
    size_t orderPos = upperQuery.find(" ORDER BY ", fromPos);

    size_t tableEnd = cleanQuery.size();
    if (wherePos != string::npos) tableEnd = wherePos;
    if (orderPos != string::npos && orderPos < tableEnd) tableEnd = orderPos;

    string tableName = trim(cleanQuery.substr(fromPos + 6, tableEnd - (fromPos + 6)));

    string conditionText = "";
    if (wherePos != string::npos) {
        size_t condStart = wherePos + 7;
        size_t condEnd = (orderPos != string::npos && orderPos > wherePos) ? orderPos : cleanQuery.size();
        conditionText = trim(cleanQuery.substr(condStart, condEnd - condStart));
    }

    string orderCol = "";
    string orderDir = "ASC";

    if (orderPos != string::npos) {
        string orderText = trim(cleanQuery.substr(orderPos + 10));
        stringstream ss(orderText);
        ss >> orderCol >> orderDir;
        orderDir = upper(orderDir);

        if (orderDir != "DESC") {
            orderDir = "ASC";
        }
    }

    if (tableName.rfind("System", 0) == 0) {
        string path = "system_catalog/" + tableName + ".bin";
        ifstream file(path, ios::binary);
        nlohmann::json rows = nlohmann::json::array();
        string line;

        if (file.is_open()) {
            while (getline(file, line)) {
                if (line.empty()) continue;
                nlohmann::json rowObj;
                rowObj["Datos_del_Catalogo"] = line;
                rows.push_back(rowObj);
            }

            file.close();
            return {true, "Consulta exitosa al catálogo del sistema", currentDatabase, rows};
        }

        return {false, "Error: no se pudo abrir la tabla de sistema especificada", currentDatabase};
    }

    if (!tableExists(currentDatabase, tableName)) {
        return {false, "Error: la tabla no existe", currentDatabase};
    }

    vector<string> colNames;
    vector<string> colTypes;
    loadColumns(currentDatabase, tableName, colNames, colTypes);

    if (colNames.empty()) {
        return {false, "Error: la tabla no tiene columnas registradas", currentDatabase};
    }

    vector<int> selectedIndexes;

    if (selectedText == "*") {
        for (int i = 0; i < (int)colNames.size(); i++) {
            selectedIndexes.push_back(i);
        }
    } else {
        vector<string> selectedCols = splitCSV(selectedText);

        for (string col : selectedCols) {
            int idx = findColumnIndex(colNames, trim(col));

            if (idx == -1) {
                return {false, "Error: columna seleccionada no existe: " + col, currentDatabase};
            }

            selectedIndexes.push_back(idx);
        }
    }

    Condition condition = parseCondition(conditionText);
    int whereColIndex = -1;

    if (condition.active) {
        whereColIndex = findColumnIndex(colNames, condition.column);

        if (whereColIndex == -1) {
            return {false, "Error: columna WHERE no existe: " + condition.column, currentDatabase};
        }
    }

    int orderColIndex = -1;
    if (!orderCol.empty()) {
        orderColIndex = findColumnIndex(colNames, orderCol);

        if (orderColIndex == -1) {
            return {false, "Error: columna ORDER BY no existe: " + orderCol, currentDatabase};
        }
    }

    struct RowData {
        vector<string> values;
    };

    vector<RowData> resultRows;

    string path = "databases/" + currentDatabase + "/" + tableName + ".bin";
    ifstream file(path, ios::binary);
    string line;

    if (file.is_open()) {
        while (getline(file, line)) {
            if (line.empty()) continue;

            string decryptedLine = descifrarDatos(line);
            vector<string> values = parseRecordValues(decryptedLine);

            if ((int)values.size() < (int)colNames.size()) continue;

            bool match = true;

            if (condition.active) {
                match = compareValues(values[whereColIndex], condition.op, condition.value);
            }

            if (match) {
                RowData row;
                row.values = values;
                resultRows.push_back(row);
            }
        }

        file.close();
    }

    if (orderColIndex != -1) {
        orderDir = upper(trim(orderDir));
        if (!orderDir.empty() && orderDir.back() == ';') {
            orderDir.pop_back();
        }

        sort(resultRows.begin(), resultRows.end(), [&](const RowData& a, const RowData& b) {
            string va = normalizeStoredValue(a.values[orderColIndex]);
            string vb = normalizeStoredValue(b.values[orderColIndex]);

            bool desc = (orderDir == "DESC");

            if (isNumber(va) && isNumber(vb)) {
                double da = stod(va);
                double db = stod(vb);

                if (desc) return da > db;
                return da < db;
            }

            if (desc) return va > vb;
            return va < vb;
        });
    }

    nlohmann::json rows = nlohmann::json::array();

    for (const RowData& row : resultRows) {
        nlohmann::json rowObj;

        for (int idx : selectedIndexes) {
            rowObj[colNames[idx]] = normalizeStoredValue(row.values[idx]);
        }

        rows.push_back(rowObj);
    }

    bool hasIndexForWhere = false;

    if (condition.active) {
        ifstream idxFile("system_catalog/SystemIndexes.bin", ios::binary);

        string idxLine;
        string idxPrefix = currentDatabase + "|" + tableName + "|";

        while (getline(idxFile, idxLine)) {
            if (!idxLine.empty() && idxLine.back() == '\r') {
                idxLine.pop_back();
            }

            if (idxLine.rfind(idxPrefix, 0) == 0) {

                size_t p1 = idxLine.find('|');
                size_t p2 = idxLine.find('|', p1 + 1);
                size_t p3 = idxLine.find('|', p2 + 1);
                size_t p4 = idxLine.find('|', p3 + 1);

                if (p1 != string::npos &&
                    p2 != string::npos &&
                    p3 != string::npos &&
                    p4 != string::npos) {

                    string indexedColumn =
                        idxLine.substr(p3 + 1, p4 - p3 - 1);

                    if (indexedColumn == condition.column &&
                        condition.op == "=") {
                        hasIndexForWhere = true;
                        break;
                    }
                }
            }
        }

        idxFile.close();
    }

    string message = "Consulta exitosa.";

    if (condition.active) {
        if (hasIndexForWhere)
            message += " (Búsqueda usando índice)";
        else
            message += " (Búsqueda secuencial)";
    }

    return {true, message, currentDatabase, rows};
    }

static QueryResponse deleteFromTable(const string& query, const string& currentDatabase) {
    if (currentDatabase.empty()) {
        return {false, "Error: primero debe ejecutar SET DATABASE", currentDatabase};
    }

    string cleanQuery = removeSemicolon(query);
    string upperQuery = upper(cleanQuery);

    size_t fromPos = upperQuery.find("FROM ");
    if (fromPos == string::npos) {
        return {false, "Error: sintaxis inválida, falta FROM", currentDatabase};
    }

    size_t wherePos = upperQuery.find(" WHERE ");

    string tableName;
    string conditionText = "";

    if (wherePos != string::npos) {
        tableName = trim(cleanQuery.substr(fromPos + 5, wherePos - (fromPos + 5)));
        conditionText = trim(cleanQuery.substr(wherePos + 7));
    } else {
        tableName = trim(cleanQuery.substr(fromPos + 5));
    }

    if (!tableExists(currentDatabase, tableName)) {
        return {false, "Error: la tabla no existe", currentDatabase};
    }

    vector<string> colNames;
    vector<string> colTypes;
    loadColumns(currentDatabase, tableName, colNames, colTypes);

    Condition condition = parseCondition(conditionText);
    int whereColIndex = -1;

    if (condition.active) {
        whereColIndex = findColumnIndex(colNames, condition.column);

        if (whereColIndex == -1) {
            return {false, "Error: la columna del WHERE no existe", currentDatabase};
        }
    }

    string path = "databases/" + currentDatabase + "/" + tableName + ".bin";
    string tempPath = "databases/" + currentDatabase + "/temp_" + tableName + ".bin";

    ifstream file(path, ios::binary);
    ofstream tempFile(tempPath, ios::binary);

    int deletedCount = 0;
    string line;

    while (getline(file, line)) {
        if (line.empty()) continue;

        string decryptedLine = descifrarDatos(line);
        vector<string> values = parseRecordValues(decryptedLine);

        bool deleteRow = true;

        if (condition.active) {
            deleteRow = compareValues(values[whereColIndex], condition.op, condition.value);
        }

        if (deleteRow) {
            deletedCount++;
        } else {
            tempFile << line << '\n';
        }
    }

    file.close();
    tempFile.close();

    remove(path.c_str());
    rename(tempPath.c_str(), path.c_str());

    updateAllIndexesForTable(currentDatabase, tableName);

    return {true, "Comando ejecutado. Filas eliminadas: " + to_string(deletedCount), currentDatabase};
}

static QueryResponse updateTable(const string& query, const string& currentDatabase) {
    if (currentDatabase.empty()) {
        return {false, "Error: primero debe ejecutar SET DATABASE", currentDatabase};
    }

    string cleanQuery = removeSemicolon(query);
    string upperQuery = upper(cleanQuery);

    size_t updatePos = upperQuery.find("UPDATE ");
    size_t setPos = upperQuery.find(" SET ");

    if (updatePos == string::npos || setPos == string::npos) {
        return {false, "Error: sintaxis inválida, falta UPDATE o SET", currentDatabase};
    }

    string tableName = trim(cleanQuery.substr(updatePos + 7, setPos - (updatePos + 7)));

    size_t wherePos = upperQuery.find(" WHERE ");

    string setPart;
    string wherePart = "";

    if (wherePos != string::npos) {
        setPart = trim(cleanQuery.substr(setPos + 5, wherePos - (setPos + 5)));
        wherePart = trim(cleanQuery.substr(wherePos + 7));
    } else {
        setPart = trim(cleanQuery.substr(setPos + 5));
    }

    if (!tableExists(currentDatabase, tableName)) {
        return {false, "Error: la tabla no existe", currentDatabase};
    }

    size_t eqSet = setPart.find('=');
    if (eqSet == string::npos) {
        return {false, "Error: sintaxis de SET inválida", currentDatabase};
    }

    string setCol = trim(setPart.substr(0, eqSet));
    string setVal = trim(setPart.substr(eqSet + 1));

    vector<string> colNames;
    vector<string> colTypes;
    loadColumns(currentDatabase, tableName, colNames, colTypes);

    int setColIndex = findColumnIndex(colNames, setCol);

    if (setColIndex == -1) {
        return {false, "Error: la columna a actualizar no existe", currentDatabase};
    }

    string typeError;
    if (!validateValueForType(setVal, colTypes[setColIndex], typeError)) {
        return {false, "Error de tipo en columna '" + setCol + "': " + typeError, currentDatabase};
    }

    Condition condition = parseCondition(wherePart);
    int whereColIndex = -1;

    if (condition.active) {
        whereColIndex = findColumnIndex(colNames, condition.column);

        if (whereColIndex == -1) {
            return {false, "Error: la columna del WHERE no existe", currentDatabase};
        }
    }

    string path = "databases/" + currentDatabase + "/" + tableName + ".bin";
    string tempPath = "databases/" + currentDatabase + "/temp_" + tableName + ".bin";

    ifstream file(path, ios::binary);
    ofstream tempFile(tempPath, ios::binary);

    int updatedCount = 0;
    string line;

    while (getline(file, line)) {
        if (line.empty()) continue;

        string decryptedLine = descifrarDatos(line);
        vector<string> values = parseRecordValues(decryptedLine);

        bool match = true;

        if (condition.active) {
            match = compareValues(values[whereColIndex], condition.op, condition.value);
        }

        if (match) {
            values[setColIndex] = setVal;
            updatedCount++;
        }

        string newLine = trim(values[0]);
        for (int i = 1; i < (int)values.size(); i++) {
            newLine += ", " + trim(values[i]);
        }

        tempFile << cifrarDatos(newLine) << '\n';
    }

    file.close();
    tempFile.close();

    remove(path.c_str());
    rename(tempPath.c_str(), path.c_str());

    updateAllIndexesForTable(currentDatabase, tableName);

    return {true, "Comando ejecutado. Filas actualizadas: " + to_string(updatedCount), currentDatabase};
}

static QueryResponse createIndex(const string& query, const string& currentDatabase) {
    if (currentDatabase.empty()) {
        return {false, "Error: primero debe ejecutar SET DATABASE", currentDatabase};
    }

    string upperQuery = upper(query);

    size_t idxPos = upperQuery.find("CREATE INDEX ");
    size_t onPos = upperQuery.find(" ON ");
    size_t parenStart = upperQuery.find("(");
    size_t parenEnd = upperQuery.find(")");
    size_t ofTypePos = upperQuery.find(" OF TYPE ");

    if (idxPos == string::npos || onPos == string::npos ||
        parenStart == string::npos || parenEnd == string::npos ||
        ofTypePos == string::npos) {
        return {false, "Error: sintaxis inválida. Uso: CREATE INDEX nombre ON tabla(columna) OF TYPE BTREE|BST", currentDatabase};
    }

    string indexName = trim(query.substr(idxPos + 13, onPos - (idxPos + 13)));
    string tableName = trim(query.substr(onPos + 4, parenStart - (onPos + 4)));
    string colName = trim(query.substr(parenStart + 1, parenEnd - (parenStart + 1)));
    string indexType = removeSemicolon(upperQuery.substr(ofTypePos + 9));

    if (indexType != "BTREE" && indexType != "BST") {
        return {false, "Error: el tipo de índice debe ser BTREE o BST", currentDatabase};
    }

    if (!tableExists(currentDatabase, tableName)) {
        return {false, "Error: la tabla no existe", currentDatabase};
    }

    // NUEVO: solo permitir un índice por tabla
    ifstream checkIdx("system_catalog/SystemIndexes.bin", ios::binary);
    string idxLine;
    string idxPrefix = currentDatabase + "|" + tableName + "|";

    while (getline(checkIdx, idxLine)) {
        if (!idxLine.empty() && idxLine.back() == '\r') {
            idxLine.pop_back();
        }

        if (idxLine.rfind(idxPrefix, 0) == 0) {
            checkIdx.close();
            return {
                false,
                "Error: la tabla '" + tableName + "' ya tiene un índice. Solo se permite un índice por tabla.",
                currentDatabase
            };
        }
    }

    checkIdx.close();

    vector<string> colNames;
    vector<string> colTypes;
    loadColumns(currentDatabase, tableName, colNames, colTypes);

    int targetColIndex = findColumnIndex(colNames, colName);

    if (targetColIndex == -1) {
        return {false, "Error: la columna no existe en la tabla", currentDatabase};
    }

    string path = "databases/" + currentDatabase + "/" + tableName + ".bin";
    ifstream file(path, ios::binary);

    IndexRecord records[1000];
    int recordCount = 0;
    int lineNumber = 1;
    string line;

    while (getline(file, line) && recordCount < 1000) {
        if (line.empty()) {
            lineNumber++;
            continue;
        }

        string decryptedLine = descifrarDatos(line);
        vector<string> values = parseRecordValues(decryptedLine);

        if (targetColIndex < (int)values.size()) {
            string colValue = normalizeStoredValue(values[targetColIndex]);

            for (int i = 0; i < recordCount; i++) {
                if (records[i].key == colValue) {
                    file.close();
                    return {
                        false,
                        "Error: no se puede crear índice, hay valores repetidos ('" + colValue + "')",
                        currentDatabase
                    };
                }
            }

            records[recordCount].key = colValue;
            records[recordCount].recordLine = lineNumber;
            recordCount++;
        }

        lineNumber++;
    }

    file.close();

    if (recordCount > 0) {
        quickSort(records, 0, recordCount - 1);
    }

    if (indexType == "BST") {
        BST bstIndex;
        for (int i = 0; i < recordCount; ++i) {
            bstIndex.insert(records[i].key, records[i].recordLine);
        }
    } else {
        BTree btreeIndex(3);
        for (int i = 0; i < recordCount; ++i) {
            btreeIndex.insert(records[i].key, records[i].recordLine);
        }
    }

    string indexPath = "databases/" + currentDatabase + "/" + indexName + ".bin";
    ofstream idxFile(indexPath, ios::binary);

    if (idxFile.is_open()) {
        idxFile << "INDICE|" << indexType << "|" << colName << "\n";

        for (int i = 0; i < recordCount; ++i) {
            idxFile << records[i].key << "|" << records[i].recordLine << "\n";
        }

        idxFile.close();
    }

    ofstream sysIdx("system_catalog/SystemIndexes.bin", ios::app | ios::binary);

    if (sysIdx.is_open()) {
        sysIdx << currentDatabase << "|"
               << tableName << "|"
               << indexName << "|"
               << colName << "|"
               << indexType << "\n";
        sysIdx.close();
    }

    return {true, "Índice '" + indexName + "' de tipo " + indexType + " creado exitosamente.", currentDatabase};
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

    if (startsWith(query, "INSERT INTO")) {
        return insertIntoTable(query, currentDatabase);
    }

    if (startsWith(query, "SELECT")) {
        return selectFromTable(query, currentDatabase);
    }

    if (startsWith(query, "DELETE FROM")) {
        return deleteFromTable(query, currentDatabase);
    }

    if (startsWith(query, "UPDATE ")) {
        return updateTable(query, currentDatabase);
    }

    if (startsWith(query, "CREATE INDEX ")) {
        return createIndex(query, currentDatabase);
    }

    return {false, "Error: sentencia no soportada por TinySQLDb", currentDatabase};
}