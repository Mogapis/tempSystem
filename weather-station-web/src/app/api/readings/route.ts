import { NextRequest, NextResponse } from "next/server";
import { getDb } from "@/lib/db";

export async function GET(req: NextRequest) {
  const { searchParams } = new URL(req.url);
  const limit = Math.min(Number(searchParams.get("limit") || 200), 2000);
  const db = getDb();
  const rows = db.prepare(`
    SELECT timestamp_utc, temperature_c, humidity_rel, source
    FROM measurements
    ORDER BY timestamp_utc DESC
    LIMIT ?
  `).all(limit);
  return NextResponse.json(rows.reverse()); // chronological
}