#!/usr/bin/env node
/* AXIOM X1 launcher — zero dependencies, clean exit, no leftover process. */
'use strict';

const http = require('http');
const fs = require('fs');
const path = require('path');
const { exec } = require('child_process');

const APP = path.resolve(__dirname, '..', 'app');
const PING = '/__axiom/ping';
const EXIT = '/__axiom/exit';
const IDLE_TIMEOUT = 5 * 60 * 1000; // safety net if the browser is killed outright
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
  let heartbeat = Date.now();
  let closing = false;
  const server = http.createServer((req, res) => {
    let urlPath;
    try { urlPath = decodeURIComponent(new URL(req.url, 'http://localhost').pathname); }
    catch { res.writeHead(400).end('Bad request'); return; }

    if (urlPath === PING) {
      heartbeat = Date.now();
      res.writeHead(200, { 'Content-Type': 'text/plain' }).end('ok');
      return;
    }
    if (urlPath === EXIT) {
      res.writeHead(200, { 'Content-Type': 'text/plain' }).end('bye');
      setTimeout(() => shutdown(server), 150);
      return;
    }
    heartbeat = Date.now();
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

  const watchdog = setInterval(() => {
    if (!closing && Date.now() - heartbeat > IDLE_TIMEOUT) shutdown(server);
  }, 15000);
  if (watchdog.unref) watchdog.unref();
  server.on('close', () => clearInterval(watchdog));

  return new Promise(resolve => server.listen(options.port || 0, options.host || '127.0.0.1', () => resolve(server)));
}

function shutdown(server) {
  if (shutdown.done) return;
  shutdown.done = true;
  try { if (server.closeAllConnections) server.closeAllConnections(); } catch (_) {}
  server.close(() => process.exit(0));
  const force = setTimeout(() => process.exit(0), 1000);
  if (force.unref) force.unref();
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
  const server = await startServer({ port: portArg ? Number(portArg.split('=')[1]) : 0 });
  const url = `http://localhost:${server.address().port}/`;

  console.log('\n  AXIOM X1 — Advanced Scientific Calculator');
  console.log('  ────────────────────────────────────────────');
  console.log(`  Running at:  ${url}`);
  console.log('  Close the calculator tab (or press Ctrl+C) to quit cleanly.\n');
  if (!noOpen) openBrowser(url);

  process.on('SIGINT', () => shutdown(server));
  process.on('SIGTERM', () => shutdown(server));
}

if (require.main === module) main().catch(e => { console.error(e); process.exit(1); });
module.exports = { startServer, APP };
