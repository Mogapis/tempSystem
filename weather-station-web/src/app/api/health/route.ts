import { NextResponse } from "next/server";
export async function GET() {
  const k = process.env.INGEST_API_KEY || "";
  return NextResponse.json({
    status: "ok",
    keyDefined: !!k,
    keyLength: k.length
  });
}