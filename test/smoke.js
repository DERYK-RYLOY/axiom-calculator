/* Zero-dependency smoke test for the AXIOM X1 package. */
'use strict';
const assert = require('assert');
const fs = require('fs');
const path = require('path');
const { startServer, APP } = require('../bin/cli.js');

(async () => {
  const server = await startServer({ port: 0 });
  const base = `http://127.0.0.1:${server.address().port}`;
  const checks = [
    ['/', 200, 'AXIOM X1'],
    ['/manifest.webmanifest', 200, 'AXIOM X1'],
    ['/sw.js', 200, 'axiom-x1'],
    ['/icons/icon-192.png', 200, null],
    ['/fonts/Jost-700.ttf', 200, null]
  ];
  for (const [route, status, contains] of checks) {
    const res = await fetch(base + route);
    assert.strictEqual(res.status, status, `${route} returned ${res.status}`);
    if (contains) {
      const text = await res.text();
      assert.ok(text.includes(contains), `${route} missing "${contains}"`);
    }
    process.stdout.write(`  ✓ ${route}\n`);
  }
  for (const f of ['index.html', 'manifest.webmanifest', 'sw.js', 'icons/icon-512.png']) {
    assert.ok(fs.existsSync(path.join(APP, f)), `missing ${f}`);
  }
  const missing = await fetch(base + '/does-not-exist');
  assert.strictEqual(missing.status, 404, '404 handling works');
  server.close();
  console.log('\n  All smoke tests passed.\n');
})().catch(e => { console.error(e); process.exit(1); });
