"use client";
import useSWR from "swr";
const fetcher = (u:string)=>fetch(u).then(r=>r.json());

export default function Home() {
  const { data } = useSWR("/api/readings?limit=1", fetcher, { refreshInterval: 1000 });
  const latest = data && data[0];
  return (
    <main className="p-6 space-y-6 font-sans">
      <h1 className="text-2xl font-bold">Weather Station</h1>
      {latest ? (
        <div className="grid gap-4 md:grid-cols-3">
          <div className="rounded-lg bg-slate-800 p-4 text-slate-100">
            <h2 className="text-xs uppercase tracking-wide text-slate-400">Temperature</h2>
            <p className="text-3xl mt-1">{latest.temperature_c.toFixed(1)}°C</p>
          </div>
          <div className="rounded-lg bg-slate-800 p-4 text-slate-100">
            <h2 className="text-xs uppercase tracking-wide text-slate-400">Humidity</h2>
            <p className="text-3xl mt-1">{latest.humidity_rel.toFixed(1)}%</p>
          </div>
          <div className="rounded-lg bg-slate-800 p-4 text-slate-100">
            <h2 className="text-xs uppercase tracking-wide text-slate-400">Updated</h2>
            <p className="mt-1 text-lg">{latest.timestamp_utc}</p>
          </div>
        </div>
      ) : <p>Loading...</p>}
      <a href="/history" className="text-blue-400 underline">History</a>
    </main>
  );
}