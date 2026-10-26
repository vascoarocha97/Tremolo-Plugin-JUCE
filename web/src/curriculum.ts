// O currículo vem de /curriculum.json (o mesmo ficheiro que o ./learn e o CMake usam).

export interface Exercise {
  id: string;
  folder: string;
  title: string;
  minutes: number;
  web: string;
  summary: string;
  module: number;
}

export interface Module {
  id: number;
  title: string;
  exercises: Exercise[];
}

let modules: Module[] = [];

export async function loadCurriculum(): Promise<void> {
  try {
    const response = await fetch("/curriculum.json", { cache: "no-store" });
    const json = (await response.json()) as { modules: Array<Omit<Module, "exercises"> & { exercises: Omit<Exercise, "module">[] }> };
    modules = json.modules.map((m) => ({ ...m, exercises: m.exercises.map((e) => ({ ...e, module: m.id })) }));
  } catch {
    modules = [];
  }
}

export function getModules(): Module[] {
  return modules;
}

export function allExercises(): Exercise[] {
  return modules.flatMap((m) => m.exercises);
}

export function findExercise(id: string): Exercise | undefined {
  return allExercises().find((e) => e.id === id);
}

export function exercisesForPage(pageId: string): Exercise[] {
  return allExercises().filter((e) => e.web === pageId);
}
