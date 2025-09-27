import { NextRequest, NextResponse } from "next/server";
import { getDb } from "@/lib/db";
import { ingestSchema } from "@/lib/validation";

export async function POST(req: NextRequest) {
  const incoming = req.headers.get("x-api-key") || "";
  const serverKey = process.env.INGEST_API_KEY || "";

  // Debug logging (remove after you confirm)
  console.log("INGEST DEBUG",
    { incomingLen: incoming.length, serverLen: serverKey.length });

  if (!serverKey) {
    return NextResponse.json({ error: "server missing key" }, { status: 500 });
  }
  if (incoming !== serverKey) {
    // Optional deeper debug (comment out after diagnosing)
    console.log("KEY HEX",
      { serverHex: Buffer.from(serverKey).toString("hex"),
        incomingHex: Buffer.from(incoming).toString("hex") });
    console.log("AUTH FAIL DEBUG", { incoming, serverKey });
  }

  let payload: unknown;
  try {
    payload = await req.json();
  } catch {
    return NextResponse.json({ error: "invalid json" }, { status: 400 });
  }

  const parsed = ingestSchema.safeParse(payload);
  if (!parsed.success) {
    return NextResponse.json({ error: parsed.error.flatten() }, { status: 422 });
  }

  const { temperature_c, humidity_rel, timestamp_utc, source } = parsed.data;
  const db = getDb();
  db.prepare(`
    INSERT INTO measurements (timestamp_utc, temperature_c, humidity_rel, source)
    VALUES (?, ?, ?, ?)
  `).run(timestamp_utc, temperature_c, humidity_rel, source);

  return NextResponse.json({ status: "ok" });
}


// import { NextRequest, NextResponse } from "next/server";
// import { getDb } from "@/lib/db";
// import { ingestSchema } from "@/lib/validation";

// // Simple in-memory rate limiter (reset on server restart)
// // For a hobby Pi project this is fine; for production use a shared store (Redis, etc.)
// const RATE_WINDOW_MS = 10_000;   // 10 seconds
// const MAX_REQUESTS_PER_SOURCE = 10;
// const recent: Record<string, number[]> = {};

// function rateLimit(source: string): boolean {
//   const now = Date.now();
//   if (!recent[source]) recent[source] = [];
//   // keep only events inside window
//   recent[source] = recent[source].filter(ts => now - ts < RATE_WINDOW_MS);
//   if (recent[source].length >= MAX_REQUESTS_PER_SOURCE) return false;
//   recent[source].push(now);
//   return true;
// }

// export async function POST(req: NextRequest) {
//   const serverKeyRaw = process.env.INGEST_API_KEY || "";
//   const incomingRaw = req.headers.get("x-api-key") || "";

//   // Trim in case of accidental whitespace
//   const serverKey = serverKeyRaw.trim();
//   const incoming = incomingRaw.trim();

//   if (!serverKey) {
//     return NextResponse.json(
//       { error: "server missing key" },
//       { status: 500 }
//     );
//   }

//   if (incoming !== serverKey) {
//     // Uncomment for occasional debugging:
//     // console.log("AUTH FAIL", { incomingLen: incoming.length, serverLen: serverKey.length });
//     return NextResponse.json({ error: "unauthorized" }, { status: 401 });
//   }

//   // Parse JSON
//   let payload: unknown;
//   try {
//     payload = await req.json();
//   } catch {
//     return NextResponse.json({ error: "invalid json" }, { status: 400 });
//   }

//   const parsed = ingestSchema.safeParse(payload);
//   if (!parsed.success) {
//     return NextResponse.json(
//       { error: parsed.error.flatten() },
//       { status: 422 }
//     );
//   }

//   const { temperature_c, humidity_rel, timestamp_utc, source } = parsed.data;

//   // Rate limit per source id (optional)
//   if (!rateLimit(source)) {
//     return NextResponse.json(
//       { error: "rate limit" },
//       { status: 429 }
//     );
//   }

//   // (Optional) Extra server-side sanity checks
//   if (humidity_rel < 0 || humidity_rel > 100) {
//     return NextResponse.json(
//       { error: "humidity out of range" },
//       { status: 422 }
//     );
//   }

//   // You can normalize timestamp here if desired:
//   // const tsDate = new Date(timestamp_utc);
//   // const iso = tsDate.toISOString();

//   try {
//     const db = getDb();
//     db.prepare(`
//       INSERT INTO measurements (timestamp_utc, temperature_c, humidity_rel, source)
//       VALUES (?, ?, ?, ?)
//     `).run(timestamp_utc, temperature_c, humidity_rel, source);
//   } catch (e: any) {
//     console.error("DB insert failed:", e);
//     return NextResponse.json(
//       { error: "db error" },
//       { status: 500 }
//     );
//   }

//   return NextResponse.json({ status: "ok" });
// }