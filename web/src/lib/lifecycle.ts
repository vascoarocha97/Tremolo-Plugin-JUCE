// Limpeza ao mudar de página: listeners, observers, animações.

let disposers: Array<() => void> = [];

export function onDispose(fn: () => void): void {
  disposers.push(fn);
}

export function disposeAll(): void {
  const current = disposers;
  disposers = [];
  for (const fn of current) {
    try {
      fn();
    } catch (error) {
      console.error(error);
    }
  }
}

/** requestAnimationFrame em ciclo, cancelado automaticamente ao sair da página. */
export function animate(frame: (timeSeconds: number) => void): void {
  let id = 0;
  const loop = (t: number) => {
    frame(t / 1000);
    id = requestAnimationFrame(loop);
  };
  id = requestAnimationFrame(loop);
  onDispose(() => cancelAnimationFrame(id));
}
