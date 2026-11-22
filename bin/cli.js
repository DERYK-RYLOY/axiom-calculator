#!/usr/bin/env node
/* AXIOM X1 launcher — zero dependencies, works on Windows/macOS/Linux. */
'use strict';

const http = require('http');
const fs = require('fs');
const path = require('path');
const { exec } = require('child_process');

const APP = path.resolve(__dirname, '..', 'app');
const MIME = {
  '.html': 'text/html; charset=utf-8',
  '.js': 'text/javascript; charset=utf-8',
  '.css': 'text/css; charset=utf-8',
  '.json': 'application/json; charset=utf-8',
  '.webmanifest': 'application/manifest+json',
  '.png': 'image/png',
  '.svg': 'image/svg+xml',
  '.ico': 'image/x-icon',
  '.ttf': 'font/ttf',
  '.woff': 'font/woff',
  '.woff2': 'font/woff2',
  '.txt': 'text/plain; charset=utf-8',
  '.md': 'text/markdown; charset=utf-8'
};

function startServer(options = {}) {
  const port = options.port || 0;
  const host = options.host || '127.0.0.1';
  const server = http.createServer((req, res) => {
    let urlPath;
    try { urlPath = decodeURIComponent(new URL(req.url, 'http://localhost').pathname); }
    catch { res.writeHead(400).end('Bad request'); return; }
    if (urlPath === '/' || urlPath === '') urlPath = '/index.html';

    const file = path.join(APP, path.normalize(urlPath).replace(/^([/\\])+/, ''));
    if (!file.startsWith(APP)) { res.writeHead(403).end('Forbidden'); return; }

    fs.stat(file, (err, st) => {
      if (err || !st.isFile()) { res.writeHead(404).end('Not found'); return; }
      res.writeHead(200, {
        'Content-Type': MIME[path.extname(file).toLowerCase()] || 'application/octet-stream',
        'Cache-Control': 'no-cache'
      });
      fs.createReadStream(file).pipe(res);
    });
  });
  return new Promise(resolve => server.listen(port, host, () => resolve(server)));
}

function openBrowser(url) {
  const cmd = process.platform === 'win32' ? `start "" "${url}"`
            : process.platform === 'darwin' ? `open "${url}"`
            : `xdg-open "${url}"`;
  exec(cmd, () => {});
}

async function main() {
  const args = process.argv.slice(2);
  const noOpen = args.includes('--no-open') || args.includes('--headless');
  const portArg = args.find(a => a.startsWith('--port='));
  const port = portArg ? Number(portArg.split('=')[1]) : 0;
  const server = await startServer({ port });
  const { port: actual } = server.address();
  const url = `http://localhost:${actual}/`;

  console.log('\n  AXIOM X1 — Advanced Scientific Calculator');
  console.log('  ────────────────────────────────────────────');
  console.log(`  Running at:  ${url}`);
  console.log('  Press Ctrl+C to stop.\n');
  if (!noOpen) openBrowser(url);

  const stop = () => { console.log('\n  Calculator stopped.'); server.close(() => process.exit(0)); };
  process.on('SIGINT', stop);
  process.on('SIGTERM', stop);
}

if (require.main === module) main().catch(e => { console.error(e); process.exit(1); });
module.exports = { startServer, APP };
