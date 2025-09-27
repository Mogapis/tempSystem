import { NextRequest, NextResponse } from "next/server";
import { getDb } from "@/lib/db";

export async function GET(req: NextRequest) {
  const { searchParams } = new URL(req.url);
  const hours = Math.min(Number(searchParams.get("hours") || 24), 168);
  const since = new Date(Date.now() - hours * 3600 * 1000).toISOString();
  const db = getDb();

  type StatsRow = {
    min_temp: number | null;
    max_temp: number | null;
    avg_temp: number | null;
    min_humidity: number | null;
    max_humidity: number | null;
    avg_humidity: number | null;
    count: number;
  };

  const row = db.prepare(`
    SELECT
      MIN(temperature_c) AS min_temp,
      MAX(temperature_c) AS max_temp,
      AVG(temperature_c) AS avg_temp,
      MIN(humidity_rel) AS min_humidity,
      MAX(humidity_rel) AS max_humidity,
      AVG(humidity_rel) AS avg_humidity,
      COUNT(*) AS count
    FROM measurements
    WHERE timestamp_utc >= ?
  `).get(since) as StatsRow | undefined;

  return NextResponse.json({ window_hours: hours, ...(row ?? {}) });
}