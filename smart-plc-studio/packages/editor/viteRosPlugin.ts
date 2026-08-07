import type { Plugin, Connect } from "vite";
import { createReadStream } from "fs";
import { readdir, readFile, stat } from "fs/promises";
import { join, relative, resolve, sep, extname } from "path";

const ROS_ROOT = resolve("/Users/junjun/ros");
const MOVEIT_ROOT = join(ROS_ROOT, "moveit_robots");

const MIME: Record<string, string> = {
  ".urdf": "application/xml",
  ".xacro": "application/xml",
  ".xml": "application/xml",
  ".dae": "model/vnd.collada+xml",
  ".stl": "model/stl",
  ".STL": "model/stl",
  ".stla": "model/stl",
  ".stlb": "model/stl",
  ".obj": "model/obj",
  ".png": "image/png",
  ".jpg": "image/jpeg",
  ".jpeg": "image/jpeg",
  ".mtl": "text/plain",
  ".txt": "text/plain",
  ".json": "application/json",
};

function contentType(path: string): string {
  return MIME[extname(path)] || "application/octet-stream";
}

async function collectPackages(dir: string, depth = 0): Promise<string[]> {
  const out: string[] = [];
  if (depth > 5) return out;
  let entries: import("fs").Dirent[];
  try {
    entries = await readdir(dir, { withFileTypes: true });
  } catch {
    return out;
  }
  for (const e of entries) {
    if (!e.isDirectory() || e.name.startsWith(".") || e.name === "node_modules" || e.name === ".git") continue;
    const sub = join(dir, e.name);
    try {
      await stat(join(sub, "package.xml"));
      out.push(sub);
    } catch {
      /* not a package */
    }
    out.push(...(await collectPackages(sub, depth + 1)));
  }
  return out;
}

async function collectAllPackages(): Promise<Map<string, string>> {
  const map = new Map<string, string>();
  let repos: import("fs").Dirent[];
  try {
    repos = await readdir(MOVEIT_ROOT, { withFileTypes: true });
  } catch {
    return map;
  }
  for (const repo of repos) {
    if (!repo.isDirectory() || repo.name.startsWith(".")) continue;
    const pkgs = await collectPackages(join(MOVEIT_ROOT, repo.name));
    for (const pkgDir of pkgs) {
      const rel = relative(MOVEIT_ROOT, pkgDir).split(sep).join("/");
      const dirName = pkgDir.split(sep).pop()!;
      const url = "/ros/moveit_robots/" + rel;
      if (!map.has(dirName)) map.set(dirName, url);
      try {
        const xml = await readFile(join(pkgDir, "package.xml"), "utf8");
        const m = /<name>\s*([^<]+)\s*<\/name>/.exec(xml);
        if (m && !map.has(m[1].trim())) map.set(m[1].trim(), url);
      } catch {
        /* ignore */
      }
    }
  }
  return map;
}

async function collectUrdfs(): Promise<
  Array<{ url: string; name: string; pkg: string; repo: string; pkgDir: string }>
> {
  const out: Array<{ url: string; name: string; pkg: string; repo: string; pkgDir: string }> = [];
  let repos: import("fs").Dirent[];
  try {
    repos = await readdir(MOVEIT_ROOT, { withFileTypes: true });
  } catch {
    return out;
  }
  for (const repo of repos) {
    if (!repo.isDirectory() || repo.name.startsWith(".")) continue;
    const repoDir = join(MOVEIT_ROOT, repo.name);
    const pkgs = await collectPackages(repoDir);
    for (const pkgDir of pkgs) {
      const rel = relative(MOVEIT_ROOT, pkgDir).split(sep);
      const pkg = rel[rel.length - 1];
      const files = await walkFiles(pkgDir, /\.urdf$/i, 6);
      for (const f of files) {
        const relPath = relative(MOVEIT_ROOT, f);
        out.push({
          url: "/ros/moveit_robots/" + relPath.split(sep).join("/"),
          name: f.split(sep).pop()!.replace(/\.urdf$/i, ""),
          pkg,
          repo: repo.name,
          pkgDir: "/ros/moveit_robots/" + relative(MOVEIT_ROOT, pkgDir).split(sep).join("/"),
        });
      }
    }
  }
  return out;
}

async function walkFiles(dir: string, re: RegExp, depth: number): Promise<string[]> {
  const out: string[] = [];
  if (depth <= 0) return out;
  let entries: import("fs").Dirent[];
  try {
    entries = await readdir(dir, { withFileTypes: true });
  } catch {
    return out;
  }
  for (const e of entries) {
    if (e.isDirectory()) {
      if (e.name.startsWith(".") || e.name === "node_modules" || e.name === ".git") continue;
      out.push(...(await walkFiles(join(dir, e.name), re, depth - 1)));
    } else if (re.test(e.name)) {
      out.push(join(dir, e.name));
    }
  }
  return out;
}

export function rosStatic(): Plugin {
  let catalog: Array<{ url: string; name: string; pkg: string; repo: string; pkgDir: string }> | null = null;
  let pkgMap: Map<string, string> | null = null;
  let catalogStamp = 0;

  async function getCatalog(force = false) {
    const now = Date.now();
    if (!force && catalog && now - catalogStamp < 60000) return catalog;
    catalog = await collectUrdfs();
    catalogStamp = now;
    return catalog;
  }

  async function getPackages(force = false) {
    const now = Date.now();
    if (!force && pkgMap && now - catalogStamp < 60000) return pkgMap;
    pkgMap = await collectAllPackages();
    catalogStamp = now;
    return pkgMap;
  }

  return {
    name: "ros-static",
    configureServer(server) {
      const mid: Connect.NextHandleFunction = async (req, res, next) => {
        const pathname = decodeURIComponent((req.url || "").split("?")[0]);
        if (pathname === "/ros-catalog.json") {
          const force = (req.url || "").includes("refresh=1");
          const list = await getCatalog(force);
          res.setHeader("Content-Type", "application/json");
          res.setHeader("Cache-Control", "no-store");
          res.end(JSON.stringify(list));
          return;
        }
        if (pathname === "/ros-packages.json") {
          const force = (req.url || "").includes("refresh=1");
          const map = await getPackages(force);
          res.setHeader("Content-Type", "application/json");
          res.setHeader("Cache-Control", "no-store");
          res.end(JSON.stringify(Object.fromEntries(map)));
          return;
        }
        if (!pathname.startsWith("/ros/")) return next();
        const rel = pathname.slice("/ros/".length);
        const target = resolve(join(ROS_ROOT, rel));
        if (!target.startsWith(resolve(ROS_ROOT))) {
          res.statusCode = 403;
          res.end("forbidden");
          return;
        }
        try {
          const st = await stat(target);
          if (st.isDirectory()) {
            const entries = await readdir(target);
            res.setHeader("Content-Type", "application/json");
            res.end(JSON.stringify({ dir: rel, entries }));
            return;
          }
          res.setHeader("Content-Type", contentType(target));
          res.setHeader("Cache-Control", "no-cache");
          createReadStream(target).pipe(res);
        } catch {
          res.statusCode = 404;
          res.end("not found: " + rel);
        }
      };
      server.middlewares.use(mid);
    },
  };
}
