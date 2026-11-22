/* Verifies the launcher exits cleanly when the calculator requests it. */
'use strict';
const assert = require('assert');
const { spawn } = require('child_process');
const path = require('path');

(async () => {
  const child = spawn(process.execPath, [path.join(__dirname, '..', 'bin', 'cli.js'), '--no-open', '--port=0'], {
    cwd: path.join(__dirname, '..'), stdio: ['ignore', 'pipe', 'pipe']
  });
  let out = '';
  child.stdout.on('data', d => { out += d.toString(); });

  const port = await new Promise((resolve, reject) => {
    const timer = setTimeout(() => reject(new Error('launcher did not start')), 8000);
    child.stdout.on('data', d => {
      const m = d.toString().match(/http:\/\/localhost:(\d+)/);
      if (m) { clearTimeout(timer); resolve(Number(m[1])); }
    });
    child.on('exit', code => reject(new Error('launcher exited early: ' + code)));
  });

  assert.strictEqual((await fetch(`http://127.0.0.1:${port}/`)).status, 200);
  assert.strictEqual((await fetch(`http://127.0.0.1:${port}/__axiom/ping`)).status, 200);
  assert.strictEqual((await fetch(`http://127.0.0.1:${port}/__axiom/exit`)).status, 200);

  const exited = await new Promise(resolve => {
    const timer = setTimeout(() => resolve(false), 5000);
    child.on('exit', code => { clearTimeout(timer); resolve(code === 0); });
  });
  assert.strictEqual(exited, true, 'launcher process did not exit after /__axiom/exit');
  console.log('  ✓ clean exit on calculator close');
})().catch(e => { console.error(e); process.exit(1); });
