// Progresso do aluno: lido de /progress.json (gerado por ./learn check) a cada 5 segundos.

export type ExerciseStatus = "done" | "in_progress" | "not_started";

export interface ExerciseProgress {
  status: ExerciseStatus;
  passed: number;
  total: number;
  failures: string[];
  lastRun: string;
}

interface ProgressFile {
  generatedAt?: string;
  branch?: string;
  exercises: Record<string, ExerciseProgress>;
}

let progress: ProgressFile = { exercises: {} };
let lastJson = "";
const listeners = new Set<() => void>();

export function onProgress(listener: () => void): () => void {
  listeners.add(listener);
  return () => listeners.delete(listener);
}

export function getProgress(): ProgressFile {
  return progress;
}

export function exerciseProgress(id: string): ExerciseProgress | undefined {
  return progress.exercises[id];
}

export function exerciseStatus(id: string): ExerciseStatus {
  const status = progress.exercises[id]?.status;
  return status === "done" || status === "in_progress" ? status : "not_started";
}

async function poll(): Promise<void> {
  try {
    const response = await fetch("/progress.json", { cache: "no-store" });
    const text = await response.text();

    if (text !== lastJson) {
      lastJson = text;
      const parsed = JSON.parse(text) as Partial<ProgressFile>;
      progress = { ...parsed, exercises: parsed.exercises ?? {} };
      for (const l of listeners) l();
    }
  } catch {
    // Sem servidor (ex.: dist aberto à mão): fica sem progresso.
  }
}

export async function startProgressPolling(): Promise<void> {
  await poll();
  window.setInterval(() => void poll(), 5000);
}
