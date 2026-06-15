# Proyecto3-Datos2
# Compilar y Ejecutar el Servidor

## Compilar

Desde la raíz del proyecto ejecutar:

```bash
g++ server/src/*.cpp -Iserver/include -std=c++17 -D_WIN32_WINNT=0x0A00 -o TinySQLDb.exe -lws2_32
```

Si la compilación fue exitosa se generará:

```text
TinySQLDb.exe
```

en la raíz del proyecto.

---

# Configuración del Entorno

## Instalar Node.js

Descargar e instalar Node.js:

https://nodejs.org

Verificar instalación:

```bash
node --version
npm --version
```

---

## Visual Studio Code

Extensiones necesarias:

### CMake Tools

```text
ms-vscode.cmake-tools
```

### Thunder Client

```text
rangav.vscode-thunder-client
```

### ES7+ React/Redux Snippets

```text
dsznajder.es7-react-js-snippets
```

### Prettier

```text
esbenp.prettier-vscode
```

---

# Probar el Backend con Thunder Client

## 1. Ejecutar el servidor

Desde la raíz del proyecto:

```bash
TinySQLDb.exe
```

Debería aparecer:

```text
Servidor TinySQLDb corriendo en http://localhost:8080
Endpoint disponible: POST http://localhost:8080/query
```

---

## 2. Crear una petición

Abrir Thunder Client:

* New Request
* Método: POST
* URL:

```text
http://localhost:8080/query
```

---

## 3. Crear una base de datos

Body → JSON

```json
{
    "query": "CREATE DATABASE Universidad;"
}
```

Respuesta esperada:

```json
{
    "success": true,
    "message": "Base de datos creada: Universidad"
}
```

---

## 4. Seleccionar una base de datos

```json
{
    "query": "SET DATABASE Universidad;"
}
```

Respuesta esperada:

```json
{
    "success": true,
    "message": "Base de datos actual: Universidad"
}
```

---

## 5. Crear una tabla

```json
{
    "query": "CREATE TABLE Estudiante(ID INTEGER, Nombre VARCHAR(30));"
}
```

Respuesta esperada:

```json
{
    "success": true,
    "message": "Tabla creada: Estudiante"
}
```

---

## 6. Verificar archivos

Después de las pruebas deben aparecer:

```text
databases/
└── Universidad/
    └── Estudiante.bin
```

y

```text
system_catalog/
├── SystemDatabases.bin
├── SystemTables.bin
├── SystemColumns.bin
└── SystemIndexes.bin
```

