// SPDX-License-Identifier: GPL-3.0-only
// The browser build in Node: boot it (witchbox1 starts stopped), press PLAY, let the band play, stop;
// expect sound, screen frames, and silence some time after stopping. No imports: the build is freestanding.
import fs from "fs";
const wasm = fs.readFileSync(process.argv[2]);
const { instance } = await WebAssembly.instantiate(wasm, {});
const ex = instance.exports, mem = ex.memory;
const run = (frames) => {
  let peak = 0, sum = 0, n = 0;
  for (let done = 0; done < frames; done += 128) {
    ex.web_render(128);
    const l = new Float32Array(mem.buffer, ex.web_out_l(), 128), r = new Float32Array(mem.buffer, ex.web_out_r(), 128);
    for (let i = 0; i < 128; i++) {
      if (!Number.isFinite(l[i]) || !Number.isFinite(r[i])) throw new Error("not a number in the output");
      peak = Math.max(peak, Math.abs(l[i]), Math.abs(r[i])); sum += l[i] * l[i] + r[i] * r[i]; n += 2;
    }
  }
  return { peak, rms: Math.sqrt(sum / n) };
};
let fail = 0;
const check = (what, ok, got) => { console.log(ok ? "  ok  " : "  FAIL", what, got ?? ""); if (!ok) fail = 1; };
ex.web_master(2800); ex.web_boot();
const z = run(44100 * 2);
check("it starts silent", z.peak < 0.004, `peak ${z.peak.toFixed(5)}`);
ex.web_buttons(1 << 10); run(4410); ex.web_buttons(0); // PLAY
const a = run(44100 * 20);
check("it plays", a.peak > 0.05 && a.peak <= 1 && a.rms > 0.005, `peak ${a.peak.toFixed(3)} rms ${a.rms.toFixed(3)}`);
check("the screen is drawn", ex.web_blits() > 100, `${ex.web_blits()} blits`);
check("PLAY is lit", (ex.web_lit_buttons() >> 10 & 1) === 1);
const fb = new Uint16Array(mem.buffer, ex.web_fb(), 240 * 240);
check("the screen is not blank", new Set(fb).size > 8, `${new Set(fb).size} colours`);
ex.web_enc(3, -2); run(4410);                         // KNOB 1: the toy's first knob
ex.web_buttons(1 << 10); run(4410); ex.web_buttons(0); // PLAY: stop
run(44100 * 20);
const q = run(44100 * 2);
check("silence after stopping", q.peak < 0.004, `peak ${q.peak.toFixed(5)}`);
check("PLAY is dark", (ex.web_lit_buttons() >> 10 & 1) === 0);
ex.web_buttons(1 << 10); run(4410); ex.web_buttons(0);
const b = run(44100 * 10);
check("and plays again", b.rms > 0.005, `rms ${b.rms.toFixed(3)}`);
ex.web_buttons(1 << 7); run(4410); ex.web_buttons(0); run(4410);   // SAVE
check("SAVE writes the project", ex.web_store_writes() >= 1 && ex.web_store_len(0) > 100, `${ex.web_store_len(0)} B`);
process.exit(fail);
