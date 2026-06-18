import { useState } from 'react';
import axios from 'axios';

function App() {
  const [query, setQuery] = useState('');
  const [history, setHistory] = useState([]);
  const [currentDb, setCurrentDb] = useState(''); // El estado para la base de datos actual

  const executeQuery = async () => {
    if (!query.trim()) return;

    try {
      const response = await axios.post('http://localhost:8080/query', {
        query: query
      });

      // Actualizamos la base de datos actual si viene en la respuesta
      if (response.data.currentDatabase !== undefined) {
        setCurrentDb(response.data.currentDatabase);
      }

      setHistory(prev => [...prev, { command: query, result: response.data }]);
      setQuery(''); 
    } catch (error) {
      const errorMessage = error.response?.data?.message || 'Error de conexión con el servidor.';
      setHistory(prev => [...prev, { command: query, result: { success: false, message: errorMessage } }]);
    }
  };

  const handleKeyDown = (e) => {
    if (e.key === 'Enter') {
      executeQuery();
    }
  };

  const renderResult = (result) => {
    if (!result.success) {
      return <div style={{ color: '#f87171' }}>Error: {result.message || 'Error desconocido'}</div>;
    }

    if (result.data && Array.isArray(result.data)) {
      
      const serverMessage = <div style={{ color: '#4ade80', marginBottom: '10px' }}>{result.message}</div>;

      if (result.data.length === 0) {
        return (
          <div>
            {serverMessage}
            <div style={{ color: '#fbbf24' }}>0 filas retornadas.</div>
          </div>
        );
      }

      const columns = Object.keys(result.data[0]);

      return (
        <div>
          {serverMessage}
          <table style={styles.table}>
            <thead>
              <tr>
                {columns.map((col) => (
                  <th key={col} style={styles.th}>{col}</th>
                ))}
              </tr>
            </thead>
            <tbody>
              {result.data.map((row, rowIndex) => (
                <tr key={rowIndex}>
                  {columns.map((col) => (
                    <td key={col} style={styles.td}>{row[col]}</td>
                  ))}
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      );
    }

    return <div style={{ color: '#4ade80' }}>{result.message || JSON.stringify(result)}</div>;
  };

  return (
    <div style={styles.container}>
      <h1 style={styles.title}>TinySQLDb - Consola Cliente</h1>
      
      <div style={styles.terminal}>
        {history.map((item, index) => (
          <div key={index} style={styles.historyItem}>
            <div style={styles.command}>&gt; {item.command}</div>
            {renderResult(item.result)}
            
            {/* NUEVO: Mostrar el tiempo de ejecución si existe */}
            {item.result.executionTimeMs !== undefined && (
              <div style={styles.time}>
                ⏱️ Tiempo de ejecución: {item.result.executionTimeMs} ms
              </div>
            )}
          </div>
        ))}
      </div>

      <div style={styles.inputArea}>
        {/* Usamos currentDb para mostrar en qué base estamos */}
        <span style={styles.prompt}>{currentDb ? `${currentDb} >` : '>'}</span>
        <input
          type="text"
          value={query}
          onChange={(e) => setQuery(e.target.value)}
          onKeyDown={handleKeyDown}
          placeholder="Escribe tu sentencia SQL aquí y presiona Enter..."
          style={styles.input}
          autoFocus
        />
        <button onClick={executeQuery} style={styles.button}>Ejecutar</button>
      </div>
    </div>
  );
}

const styles = {
  container: { padding: '20px', fontFamily: 'monospace', backgroundColor: '#121212', minHeight: '100vh', color: '#e5e5e5' },
  title: { textAlign: 'center', color: '#38bdf8' },
  terminal: { backgroundColor: '#000', padding: '15px', height: '60vh', overflowY: 'auto', borderRadius: '5px', marginBottom: '20px', border: '1px solid #333' },
  historyItem: { marginBottom: '15px', paddingBottom: '10px', borderBottom: '1px dashed #333' },
  command: { color: '#fbbf24', fontWeight: 'bold', marginBottom: '5px' },
  inputArea: { display: 'flex', gap: '10px', alignItems: 'center' },
  prompt: { color: '#fbbf24', fontSize: '1.2rem', fontWeight: 'bold' },
  input: { flex: 1, padding: '10px', fontSize: '1rem', backgroundColor: '#1e1e1e', color: '#fff', border: '1px solid #333', borderRadius: '4px', fontFamily: 'monospace' },
  button: { padding: '10px 20px', fontSize: '1rem', backgroundColor: '#38bdf8', color: '#000', border: 'none', borderRadius: '4px', cursor: 'pointer', fontWeight: 'bold' },
  table: { width: '100%', borderCollapse: 'collapse', marginTop: '10px', marginBottom: '10px', backgroundColor: '#1e1e1e' },
  th: { border: '1px solid #333', padding: '8px', backgroundColor: '#2a2a2a', color: '#38bdf8', textAlign: 'left' },
  td: { border: '1px solid #333', padding: '8px', color: '#e5e5e5' },
  time: { color: '#888', fontSize: '0.85rem', marginTop: '5px', fontStyle: 'italic' } // Estilo para el tiempo
};

export default App;