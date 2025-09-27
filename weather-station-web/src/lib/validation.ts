import { z } from "zod";

export const ingestSchema = z.object({
  temperature_c: z.number().gte(-60).lte(90),
  humidity_rel: z.number().gte(0).lte(100),
  timestamp_utc: z.string().refine(v => !Number.isNaN(Date.parse(v)), "Invalid timestamp"),
  source: z.string().min(1).max(40)
});