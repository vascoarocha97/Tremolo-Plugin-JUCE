import { defineConfig, type Plugin } from "vite";
import { existsSync, readFileSync } from "node:fs";
import { resolve } from "node:path";
import { fileURLToPath } from "node:url";
import type { IncomingMessage, ServerResponse } from "node:http";

// A raiz do repositório (onde estão curriculum.json e progress.json).
const repoRoot = fileURLToPath(new URL("..", import.meta.url));

/** Serve /curriculum.json e /progress.json diretamente da raiz do repositório, sem cache.
    O progress.json é gerado pelo ./learn check e muda enquanto a web app está aberta. */
function repoFiles(): Plugin {
  const handler = (req: IncomingMessage, res: ServerResponse, next: () => void) => {
    const url = (req.url ?? "").split("?")[0];
    const files: Record<string, string> = {
      "/curriculum.json": resolve(repoRoot, "curriculum.json"),
      "/progress.json": resolve(repoRoot, "progress.json"),
    };

    const path = files[url];

    if (!path) {
      next();
      return;
    }

    res.setHeader("Content-Type", "application/json; charset=utf-8");
    res.setHeader("Cache-Control", "no-store");

    if (existsSync(path)) {
      res.end(readFileSync(path, "utf-8"));
    } else {
      // Ainda não houve nenhum ./learn check: progresso vazio.
      res.end(url === "/progress.json" ? JSON.stringify({ exercises: {} }) : "{}");
    }
  };

  return {
    name: "learn-juce-repo-files",
    configureServer(server) {
      server.middlewares.use(handler);
    },
    configurePreviewServer(server) {
      server.middlewares.use(handler);
    },
    // No build de produção, o currículo vai junto (o progresso continua a vir do servidor).
    generateBundle() {
      this.emitFile({
        type: "asset",
        fileName: "curriculum.json",
        source: readFileSync(resolve(repoRoot, "curriculum.json"), "utf-8"),
      });
    },
  };
}

export default defineConfig({
  plugins: [repoFiles()],
  // O ./learn web abre o browser (npm run dev -- --open).
  server: { port: 5173, strictPort: true },
  preview: { port: 5174, strictPort: true },
  build: { target: "es2022" },
});
