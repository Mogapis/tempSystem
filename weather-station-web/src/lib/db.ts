import Database from "better-sqlite3";

let db: Database.Database | null = null;

export function getDb() {
  if (!db) {
    db = new Database(process.env.SQLITE_PATH || "weather.sqlite");
    db.pragma("journal_mode = WAL");
    db.exec(`
      CREATE TABLE IF NOT EXISTS measurements (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        timestamp_utc TEXT NOT NULL,
        temperature_c REAL NOT NULL,
        humidity_rel REAL NOT NULL,
        source TEXT NOT NULL,
        quality_flag INTEGER DEFAULT 0
      );
      CREATE INDEX IF NOT EXISTS idx_measurements_ts ON measurements(timestamp_utc);
    `);
  }
  console.log("[DB] Opening DB at path:", process.env.SQLITE_PATH || "weather.sqlite");
  return db;
  
}