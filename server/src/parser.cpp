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

//Encriptado César
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

static bool isValidInteger(const string& val) {
    if (val.empty()) return false;
    size_t start = (val[0] == '-') ? 1 : 0; // Soporta números negativos
    if (start == val.length()) return false;
    for (size_t i = start; i < val.length(); i++) {
        if (!isdigit(val[i])) return false;
    }
    return true;
}

static bool isValidDouble(const string& val) {
    if (val.empty()) return false;
    size_t start = (val[0] == '-') ? 1 : 0;
    bool hasDot = false;
    if (start == val.length()) return false;
    for (size_t i = start; i < val.length(); i++) {
        if (val[i] == '.') {
            if (hasDot) return false; // No puede tener dos puntos
            hasDot = true;
        } else if (!isdigit(val[i])) {
            return false;
        }
    }
    return true;
}

static QueryResponse insertIntoTable(const string& query, const string& currentDatabase) {
    if (currentDatabase.empty()) return {false, "Error: primero debe ejecutar SET DATABASE", currentDatabase};

    size_t nameStart = string("INSERT INTO").size();
    string upperQuery = query;
    transform(upperQuery.begin(), upperQuery.end(), upperQuery.begin(), ::toupper);
    
    size_t valuesPos = upperQuery.find(" VALUES");
    if (valuesPos == string::npos) return {false, "Error: sintaxis inválida, falta la palabra VALUES", currentDatabase};

    string tableName = trim(query.substr(nameStart, valuesPos - nameStart));
    
    size_t parenStart = query.find("(", valuesPos);
    size_t parenEnd = query.rfind(")");

    if (parenStart == string::npos || parenEnd == string::npos || parenStart >= parenEnd) {
        return {false, "Error: sintaxis inválida, faltan los paréntesis () en VALUES", currentDatabase};
    }

    string valuesText = query.substr(parenStart + 1, parenEnd - parenStart - 1);

    if (tableName.empty()) return {false, "Error: nombre de tabla vacío", currentDatabase};
    if (!tableExists(currentDatabase, tableName)) return {false, "Error: la tabla no existe en la base de datos", currentDatabase};

    string expectedTypes[100];
    int colCount = 0;
    ifstream catFile("system_catalog/SystemColumns.bin", ios::binary);
    string line;
    string prefix = currentDatabase + "|" + tableName + "|";
    
    while (getline(catFile, line)) {
        if (line.rfind(prefix, 0) == 0) {
            string rest = line.substr(prefix.length());
            size_t p1 = rest.find('|');
            if (p1 != string::npos) {
                size_t p2 = rest.find('|', p1 + 1);
                // Extraemos el tipo de dato de la línea del catálogo
                string colType = (p2 == string::npos) ? rest.substr(p1 + 1) : rest.substr(p1 + 1, p2 - p1 - 1);
                if (!colType.empty() && colType.back() == '\r') colType.pop_back(); // Limpieza por si hay saltos de línea ocultos
                expectedTypes[colCount] = trim(colType);
                colCount++;
            }
        }
    }
    catFile.close();

    string actualValues[100];
    int valCount = 0;
    size_t start = 0;
    while (start <= valuesText.length()) {
        size_t comma = valuesText.find(',', start);
        if (comma == string::npos) {
            actualValues[valCount++] = trim(valuesText.substr(start));
            break;
        } else {
            actualValues[valCount++] = trim(valuesText.substr(start, comma - start));
            start = comma + 1;
        }
    }

    if (valCount != colCount) {
        return {false, "Error de columnas: La tabla espera " + to_string(colCount) + " valores, pero se enviaron " + to_string(valCount), currentDatabase};
    }

    for (int i = 0; i < colCount; i++) {
        string t = expectedTypes[i];
        transform(t.begin(), t.end(), t.begin(), ::toupper);
        string v = actualValues[i];

        if (t.find("INT") != string::npos) {
            if (!isValidInteger(v)) {
                return {false, "Error de tipo (Columna " + to_string(i+1) + "): '" + v + "' no es un INTEGER válido.", currentDatabase};
            }
        }
        else if (t.find("DOUBLE") != string::npos || t.find("FLOAT") != string::npos) {
            if (!isValidDouble(v)) {
                return {false, "Error de tipo (Columna " + to_string(i+1) + "): '" + v + "' no es un DOUBLE válido.", currentDatabase};
            }
        }
        else if (t.find("VARCHAR") != string::npos || t.find("DATETIME") != string::npos) {
            if (v.empty() || v.front() != '\'' || v.back() != '\'') {
                return {false, "Error de tipo (Columna " + to_string(i+1) + "): '" + v + "' es un texto y debe ir entre comillas simples.", currentDatabase};
            }
        }
    }

    string datosEncriptados = cifrarDatos(valuesText);
    string path = "databases/" + currentDatabase + "/" + tableName + ".bin";
    ofstream file(path, ios::app | ios::binary);
    
    if (!file.is_open()) return {false, "Error: No se pudo abrir el archivo para insertar los datos", currentDatabase};

    file << datosEncriptados << '\n';
    file.close();

    return {true, "Fila insertada exitosamente en la tabla " + tableName, currentDatabase};
}
static QueryResponse selectFromTable(const string& query, const string& currentDatabase) {
    if (currentDatabase.empty()) return {false, "Error: primero debe ejecutar SET DATABASE", currentDatabase};

    string upperQuery = query;
    transform(upperQuery.begin(), upperQuery.end(), upperQuery.begin(), ::toupper);

    size_t fromPos = upperQuery.find("FROM ");
    if (fromPos == string::npos) return {false, "Error: sintaxis inválida, falta FROM", currentDatabase};

    size_t wherePos = upperQuery.find(" WHERE ");
    string tableName = "";
    string condition = "";

    if (wherePos != string::npos) {
        tableName = trim(query.substr(fromPos + 5, wherePos - (fromPos + 5)));
        condition = trim(query.substr(wherePos + 7));
        if (!condition.empty() && condition.back() == ';') condition.pop_back();
    } else {
        tableName = trim(query.substr(fromPos + 5));
        if (!tableName.empty() && tableName.back() == ';') tableName.pop_back();
    }

    if (!tableExists(currentDatabase, tableName)) return {false, "Error: la tabla no existe", currentDatabase};

    string whereCol = "", whereVal = "";
    if (!condition.empty()) {
        size_t eqPos = condition.find('=');
        if (eqPos != string::npos) {
            whereCol = trim(condition.substr(0, eqPos));
            whereVal = trim(condition.substr(eqPos + 1));
            // Quitamos las comillas
            if (whereVal.size() >= 2 && whereVal.front() == '\'' && whereVal.back() == '\'') {
                whereVal = whereVal.substr(1, whereVal.size() - 2);
            }
        }
    }

    string cols[100];
    int colCount = 0;
    int whereColIndex = -1;
    ifstream catFile("system_catalog/SystemColumns.bin", ios::binary);
    string line;
    string prefix = currentDatabase + "|" + tableName + "|";
    while (getline(catFile, line)) {
        if (line.rfind(prefix, 0) == 0) {
            string rest = line.substr(prefix.length());
            size_t pipe = rest.find('|');
            if (pipe != string::npos) {
                string cName = rest.substr(0, pipe);
                cols[colCount] = cName;
                if (cName == whereCol) whereColIndex = colCount;
                colCount++;
            }
        }
    }
    catFile.close();

    if (!condition.empty() && whereColIndex == -1) return {false, "Error: columna WHERE no existe", currentDatabase};

    string indexName = "";
    string indexType = "";
    if (!whereCol.empty()) {
        ifstream idxCat("system_catalog/SystemIndexes.bin", ios::binary);
        while (getline(idxCat, line)) {
            string searchPrefix = currentDatabase + "|" + tableName + "|";
            if (line.rfind(searchPrefix, 0) == 0) {
                int p1 = line.find('|');
                int p2 = line.find('|', p1 + 1);
                int p3 = line.find('|', p2 + 1);
                int p4 = line.find('|', p3 + 1);
                
                string idxName = line.substr(p2 + 1, p3 - p2 - 1);
                string cName = line.substr(p3 + 1, p4 - p3 - 1);
                string type = line.substr(p4 + 1);
                
                if (cName == whereCol) {
                    indexName = idxName;
                    indexType = type;
                    break;
                }
            }
        }
        idxCat.close();
    }

    string path = "databases/" + currentDatabase + "/" + tableName + ".bin";
    nlohmann::json rows = nlohmann::json::array();
    bool usedIndex = false;

    if (!indexName.empty()) {
        usedIndex = true;
        string indexPath = "databases/" + currentDatabase + "/" + indexName + ".bin";
        ifstream idxFile(indexPath, ios::binary);
        int targetLine = -1;
        
        if (idxFile.is_open()) {
            getline(idxFile, line);
            while (getline(idxFile, line)) {
                size_t pipe = line.find('|');
                if (pipe != string::npos) {
                    string key = line.substr(0, pipe);
                    if (key == whereVal) {
                        targetLine = stoi(line.substr(pipe + 1));
                        break;
                    }
                }
            }
            idxFile.close();
        }

        if (targetLine != -1) {
            ifstream file(path, ios::binary);
            int currentLine = 1;
            while (getline(file, line)) {
                if (currentLine == targetLine) {
                    string decryptedLine = descifrarDatos(line);
                    nlohmann::json rowObj;
                    size_t start = 0;
                    for (int i = 0; i < colCount; ++i) {
                        size_t comma = decryptedLine.find(',', start);
                        string value;
                        if (comma == string::npos) value = decryptedLine.substr(start);
                        else { value = decryptedLine.substr(start, comma - start); start = comma + 1; }
                        value = trim(value);
                        if (value.size() >= 2 && value.front() == '\'' && value.back() == '\'') {
                            value = value.substr(1, value.size() - 2);
                        }
                        rowObj[cols[i]] = value;
                    }
                    rows.push_back(rowObj);
                    break;
                }
                currentLine++;
            }
            file.close();
        }
    } else {
        ifstream file(path, ios::binary);
        if (file.is_open()) {
            while (getline(file, line)) {
                if (line.empty()) continue;
                string decryptedLine = descifrarDatos(line);
                string values[100];
                size_t start = 0;
                for (int i = 0; i < colCount; ++i) {
                    size_t comma = decryptedLine.find(',', start);
                    if (comma == string::npos) values[i] = decryptedLine.substr(start);
                    else { values[i] = decryptedLine.substr(start, comma - start); start = comma + 1; }
                    values[i] = trim(values[i]);
                    if (values[i].size() >= 2 && values[i].front() == '\'' && values[i].back() == '\'') {
                        values[i] = values[i].substr(1, values[i].size() - 2);
                    }
                }

                bool match = true;
                if (!condition.empty()) {
                    match = (values[whereColIndex] == whereVal);
                }

                if (match) {
                    nlohmann::json rowObj;
                    for (int i = 0; i < colCount; ++i) {
                        rowObj[cols[i]] = values[i];
                    }
                    rows.push_back(rowObj);
                }
            }
            file.close();
        }
    }

    string msj = "Consulta exitosa.";
    if (usedIndex) {
        msj += " (Búsqueda ULTRA RÁPIDA usando índice '" + indexName + "' tipo " + indexType + ")";
    } else if (!condition.empty()) {
        msj += " (Búsqueda SECUENCIAL lenta)";
    }

    return {true, msj, currentDatabase, rows};
}

static QueryResponse deleteFromTable(const string& query, const string& currentDatabase) {
    if (currentDatabase.empty()) {
        return {false, "Error: primero debe ejecutar SET DATABASE", currentDatabase};
    }

    string upperQuery = query;
    transform(upperQuery.begin(), upperQuery.end(), upperQuery.begin(), ::toupper);

    size_t fromPos = upperQuery.find("FROM ");
    if (fromPos == string::npos) return {false, "Error: sintaxis inválida, falta FROM", currentDatabase};

    size_t wherePos = upperQuery.find(" WHERE ");
    string tableName;
    string condition = "";

    if (wherePos != string::npos) {
        tableName = trim(query.substr(fromPos + 5, wherePos - (fromPos + 5)));
        condition = trim(query.substr(wherePos + 7));
        if (!condition.empty() && condition.back() == ';') condition.pop_back();
    } else {
        tableName = trim(query.substr(fromPos + 5));
        if (!tableName.empty() && tableName.back() == ';') tableName.pop_back();
    }

    if (!tableExists(currentDatabase, tableName)) return {false, "Error: la tabla no existe", currentDatabase};

    string whereCol = "", whereVal = "";
    if (!condition.empty()) {
        size_t eqPos = condition.find('=');
        if (eqPos != string::npos) {
            whereCol = trim(condition.substr(0, eqPos));
            whereVal = trim(condition.substr(eqPos + 1));
            if (whereVal.size() >= 2 && whereVal.front() == '\'' && whereVal.back() == '\'') {
                whereVal = whereVal.substr(1, whereVal.size() - 2);
            }
        }
    }

    string cols[100];
    int colCount = 0;
    int targetColIndex = -1;
    ifstream catFile("system_catalog/SystemColumns.bin", ios::binary);
    string line;
    string prefix = currentDatabase + "|" + tableName + "|";
    
    while (getline(catFile, line)) {
        if (line.rfind(prefix, 0) == 0) {
            string rest = line.substr(prefix.length());
            size_t pipe = rest.find('|');
            if (pipe != string::npos) {
                string colName = rest.substr(0, pipe);
                cols[colCount] = colName;
                if (colName == whereCol) targetColIndex = colCount;
                colCount++;
            }
        }
    }
    catFile.close();

    if (!condition.empty() && targetColIndex == -1) {
        return {false, "Error: la columna del WHERE no existe en esta tabla", currentDatabase};
    }

    string path = "databases/" + currentDatabase + "/" + tableName + ".bin";
    string tempPath = "databases/" + currentDatabase + "/temp_" + tableName + ".bin";
    ifstream file(path, ios::binary);
    ofstream tempFile(tempPath, ios::binary);

    int deletedCount = 0;

    if (file.is_open()) {
        while (getline(file, line)) {
            if (line.empty()) continue;

            string decryptedLine = descifrarDatos(line);
            bool keepRow = true;

            if (!condition.empty()) {
                size_t start = 0;
                string colValue;
                for (int i = 0; i <= targetColIndex; ++i) {
                    size_t comma = decryptedLine.find(',', start);
                    if (comma == string::npos) {
                        colValue = decryptedLine.substr(start);
                    } else {
                        colValue = decryptedLine.substr(start, comma - start);
                        start = comma + 1;
                    }
                }
                
                colValue = trim(colValue);
                if (colValue.size() >= 2 && colValue.front() == '\'' && colValue.back() == '\'') {
                    colValue = colValue.substr(1, colValue.size() - 2);
                }
                
                if (colValue == whereVal) {
                    keepRow = false;
                    deletedCount++;
                }
            } else {
                keepRow = false;
                deletedCount++;
            }

            if (keepRow) {
                tempFile << line << '\n';
            }
        }
        file.close();
        tempFile.close();

        remove(path.c_str());
        rename(tempPath.c_str(), path.c_str());
    }

    return {true, "Comando ejecutado. Filas eliminadas: " + to_string(deletedCount), currentDatabase};
}

static QueryResponse updateTable(const string& query, const string& currentDatabase) {
    if (currentDatabase.empty()) return {false, "Error: primero debe ejecutar SET DATABASE", currentDatabase};

    string upperQuery = query;
    transform(upperQuery.begin(), upperQuery.end(), upperQuery.begin(), ::toupper);

    size_t updatePos = upperQuery.find("UPDATE ");
    size_t setPos = upperQuery.find(" SET ");
    if (updatePos == string::npos || setPos == string::npos) {
        return {false, "Error: sintaxis inválida, falta UPDATE o SET", currentDatabase};
    }

    string tableName = trim(query.substr(updatePos + 7, setPos - (updatePos + 7)));
    
    size_t wherePos = upperQuery.find(" WHERE ");
    string setPart = "", wherePart = "";

    if (wherePos != string::npos) {
        setPart = trim(query.substr(setPos + 5, wherePos - (setPos + 5)));
        wherePart = trim(query.substr(wherePos + 7));
        if (!wherePart.empty() && wherePart.back() == ';') wherePart.pop_back();
    } else {
        setPart = trim(query.substr(setPos + 5));
        if (!setPart.empty() && setPart.back() == ';') setPart.pop_back();
    }

    if (!tableExists(currentDatabase, tableName)) return {false, "Error: la tabla no existe", currentDatabase};

    size_t eqSet = setPart.find('=');
    if (eqSet == string::npos) return {false, "Error: sintaxis de SET inválida", currentDatabase};
    string setCol = trim(setPart.substr(0, eqSet));
    string setVal = trim(setPart.substr(eqSet + 1)); // Este lo guardaremos tal cual lo escriba el usuario

    string whereCol = "", whereVal = "";
    if (!wherePart.empty()) {
        size_t eqWhere = wherePart.find('=');
        if (eqWhere != string::npos) {
            whereCol = trim(wherePart.substr(0, eqWhere));
            whereVal = trim(wherePart.substr(eqWhere + 1));
            if (whereVal.size() >= 2 && whereVal.front() == '\'' && whereVal.back() == '\'') {
                whereVal = whereVal.substr(1, whereVal.size() - 2);
            }
        }
    }

    int setColIndex = -1, whereColIndex = -1, colCount = 0;
    ifstream catFile("system_catalog/SystemColumns.bin", ios::binary);
    string line;
    string prefix = currentDatabase + "|" + tableName + "|";
    
    while (getline(catFile, line)) {
        if (line.rfind(prefix, 0) == 0) {
            string rest = line.substr(prefix.length());
            size_t pipe = rest.find('|');
            if (pipe != string::npos) {
                string colName = rest.substr(0, pipe);
                if (colName == setCol) setColIndex = colCount;
                if (colName == whereCol) whereColIndex = colCount;
                colCount++;
            }
        }
    }
    catFile.close();

    if (setColIndex == -1) return {false, "Error: la columna a actualizar (SET) no existe", currentDatabase};
    if (!wherePart.empty() && whereColIndex == -1) return {false, "Error: la columna del WHERE no existe", currentDatabase};

    string path = "databases/" + currentDatabase + "/" + tableName + ".bin";
    string tempPath = "databases/" + currentDatabase + "/temp_" + tableName + ".bin";
    ifstream file(path, ios::binary);
    ofstream tempFile(tempPath, ios::binary);

    int updatedCount = 0;

    if (file.is_open()) {
        while (getline(file, line)) {
            if (line.empty()) continue;

            string decryptedLine = descifrarDatos(line);
            
            string values[100];
            size_t start = 0;
            for (int i = 0; i < colCount; ++i) {
                size_t comma = decryptedLine.find(',', start);
                if (comma == string::npos) {
                    values[i] = decryptedLine.substr(start);
                } else {
                    values[i] = decryptedLine.substr(start, comma - start);
                    start = comma + 1;
                }
            }

            bool match = true;
            if (!wherePart.empty()) {
                string currentWhereVal = trim(values[whereColIndex]);
                if (currentWhereVal.size() >= 2 && currentWhereVal.front() == '\'' && currentWhereVal.back() == '\'') {
                    currentWhereVal = currentWhereVal.substr(1, currentWhereVal.size() - 2);
                }
                match = (currentWhereVal == whereVal);
            }

            if (match) {
                values[setColIndex] = " " + setVal;
                updatedCount++;
            }

            string newLine = trim(values[0]);
            for (int i = 1; i < colCount; ++i) {
                newLine += ", " + trim(values[i]);
            }

            tempFile << cifrarDatos(newLine) << '\n';
        }
        file.close();
        tempFile.close();

        remove(path.c_str());
        rename(tempPath.c_str(), path.c_str());
    }

    return {true, "Comando ejecutado. Filas actualizadas: " + to_string(updatedCount), currentDatabase};
}

static QueryResponse createIndex(const string& query, const string& currentDatabase) {
    if (currentDatabase.empty()) return {false, "Error: primero debe ejecutar SET DATABASE", currentDatabase};

    string upperQuery = query;
    transform(upperQuery.begin(), upperQuery.end(), upperQuery.begin(), ::toupper);

    size_t idxPos = upperQuery.find("CREATE INDEX ");
    size_t onPos = upperQuery.find(" ON ");
    size_t parenStart = upperQuery.find("(");
    size_t parenEnd = upperQuery.find(")");
    size_t ofTypePos = upperQuery.find(" OF TYPE ");

    if (idxPos == string::npos || onPos == string::npos || parenStart == string::npos || parenEnd == string::npos || ofTypePos == string::npos) {
        return {false, "Error: sintaxis inválida. Uso: CREATE INDEX nombre ON tabla(columna) OF TYPE BTREE|BST", currentDatabase};
    }

    string indexName = trim(query.substr(idxPos + 13, onPos - (idxPos + 13)));
    string tableName = trim(query.substr(onPos + 4, parenStart - (onPos + 4)));
    string colName = trim(query.substr(parenStart + 1, parenEnd - (parenStart + 1)));
    string indexType = trim(upperQuery.substr(ofTypePos + 9));
    
    if (!indexType.empty() && indexType.back() == ';') indexType.pop_back();
    indexType = trim(indexType);

    if (indexType != "BTREE" && indexType != "BST") {
        return {false, "Error: El tipo de índice debe ser BTREE o BST", currentDatabase};
    }

    if (!tableExists(currentDatabase, tableName)) return {false, "Error: la tabla no existe", currentDatabase};

    int targetColIndex = -1;
    int colCount = 0;
    ifstream catFile("system_catalog/SystemColumns.bin", ios::binary);
    string line;
    string prefix = currentDatabase + "|" + tableName + "|";
    
    while (getline(catFile, line)) {
        if (line.rfind(prefix, 0) == 0) {
            string rest = line.substr(prefix.length());
            size_t pipe = rest.find('|');
            if (pipe != string::npos) {
                if (rest.substr(0, pipe) == colName) targetColIndex = colCount;
                colCount++;
            }
        }
    }
    catFile.close();

    if (targetColIndex == -1) return {false, "Error: la columna no existe en la tabla", currentDatabase};

    string path = "databases/" + currentDatabase + "/" + tableName + ".bin";
    ifstream file(path, ios::binary);
    
    IndexRecord records[1000];
    int recordCount = 0;
    int lineNumber = 1;

    if (file.is_open()) {
        while (getline(file, line) && recordCount < 1000) {
            if (line.empty()) { lineNumber++; continue; }

            string decryptedLine = descifrarDatos(line);
            string colValue;
            size_t start = 0;
            
            for (int i = 0; i <= targetColIndex; ++i) {
                size_t comma = decryptedLine.find(',', start);
                if (comma == string::npos) colValue = decryptedLine.substr(start);
                else { colValue = decryptedLine.substr(start, comma - start); start = comma + 1; }
            }
            
            colValue = trim(colValue);
            if (colValue.size() >= 2 && colValue.front() == '\'' && colValue.back() == '\'') {
                colValue = colValue.substr(1, colValue.size() - 2);
            }
            
            for (int i = 0; i < recordCount; ++i) {
                if (records[i].key == colValue) {
                    return {false, "Error: no se puede crear índice, hay valores repetidos ('" + colValue + "')", currentDatabase};
                }
            }

            records[recordCount].key = colValue;
            records[recordCount].recordLine = lineNumber;
            recordCount++;
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

    ofstream sysIdx("system_catalog/SystemIndexes.bin", ios::app | ios::binary);
    if (sysIdx.is_open()) {
        sysIdx << currentDatabase << "|" << tableName << "|" << indexName << "|" << colName << "|" << indexType << "\n";
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

    return {false, "Error: sentencia no soportada por Estudiante B", currentDatabase};
}