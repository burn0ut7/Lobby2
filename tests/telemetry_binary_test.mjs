// Run with benchmark.json from --binary-benchmark; no browser/game mutations.
import { readFileSync, writeFileSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { performance } from 'node:perf_hooks';
import assert from 'node:assert/strict';
import { decodeTelemetry } from '../integrations/StatsIngest/web/telemetry-binary.mjs';

const manifestPath = process.argv[2];
const manifest = JSON.parse(readFileSync(manifestPath, 'utf8'));
const corpus = manifest.files.map(file => ({
  text: readFileSync(file.source, 'utf8'),
  bytes: readFileSync(join(dirname(manifestPath), file.binary)),
}));
for (const { text, bytes } of corpus)
  assert.deepStrictEqual(JSON.parse(JSON.stringify(decodeTelemetry(bytes))), JSON.parse(JSON.stringify(JSON.parse(text))));

let rejected = 0;
const example = corpus.find(({ bytes }) => bytes.readInt32LE(16) >= 2).bytes;
function reject(change) {
  const bytes = Buffer.from(example);
  assert.throws(() => decodeTelemetry(change(bytes) ?? bytes));
  rejected++;
}
for (const end of [0, 4, 23, 24, example.length - 1]) reject(b => b.subarray(0, end));
reject(b => { b.writeInt32LE(0, 0); });
reject(b => { b.writeInt32LE(2, 4); });
reject(b => { b.writeInt32LE(-1, 8); });
reject(b => { b.writeInt32LE(0x7fffffff, 16); });
reject(b => Buffer.concat([b, Buffer.from([0])]));
const tableOffset = 24 + example.readInt32LE(8) + example.readInt32LE(12);
reject(b => { b.writeInt32LE(-1, tableOffset + 4); });
reject(b => { b.writeInt32LE(0x20000, tableOffset + 8); });
reject(b => { b.writeInt32LE(b.readInt32LE(tableOffset), tableOffset + 16); });
reject(b => { b.writeFloatLE(NaN, tableOffset + b.readInt32LE(16) * 16); });

function measure(run) {
  for (let i = 0; i < 5; i++) run();
  const samples = [];
  for (let i = 0; i < 15; i++) { const start = performance.now(); run(); samples.push(performance.now() - start); }
  samples.sort((a, b) => a - b);
  return { medianMs: samples[7], minMs: samples[0], maxMs: samples[14] };
}
const report = {
  chunks: corpus.length, exactLogicalRoundTrips: corpus.length, malformedPacketsRejected: rejected,
  jsonParse: measure(() => { for (const { text } of corpus) JSON.parse(text); }),
  binaryDecode: measure(() => { for (const { bytes } of corpus) decodeTelemetry(bytes); }),
  scope: `Node ${process.version}, same browser-compatible decoder; memory-only corpus, 5 warmups/15 runs. JSON baseline parses only; binary additionally checks framing and numeric timelines. Not a browser rendering or network benchmark.`,
};
writeFileSync(join(dirname(manifestPath), 'javascript-benchmark.json'), JSON.stringify(report, null, 2));
console.log(JSON.stringify(report, null, 2));
