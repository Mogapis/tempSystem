"use client";
import useSWR from "swr";
const fetcher=(u:string)=>fetch(u).then(r=>r.json());

export default function History() {
  const { data } = useSWR("/api/readings?limit=500", fetcher, { refreshInterval: 30000 });
  return (
    <main className="p-6 font-sans">
      <h1 className="text-xl font-semibold mb-4">Recent Readings</h1>
      {!data && <p>Loading...</p>}
      {data && (
        <div className="overflow-x-auto">
          <table className="w-full text-sm">
            <thead className="bg-slate-800 text-slate-200">
              <tr>
                <th className="px-3 py-2 text-left">Time (UTC)</th>
                <th className="px-3 py-2 text-left">Temp (°C)</th>
                <th className="px-3 py-2 text-left">Humidity (%)</th>
                <th className="px-3 py-2 text-left">Source</th>
              </tr>
            </thead>
            <tbody>
              {data.map((r:any)=>(
                <tr key={r.timestamp_utc + r.source} className="odd:bg-slate-900 even:bg-slate-800/60">
                  <td className="px-3 py-1">{r.timestamp_utc}</td>
                  <td className="px-3 py-1">{r.temperature_c.toFixed(2)}</td>
                  <td className="px-3 py-1">{r.humidity_rel.toFixed(1)}</td>
                  <td className="px-3 py-1">{r.source}</td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      )}
    </main>
  );
}