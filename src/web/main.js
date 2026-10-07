const FONTS = [
  (size) => `bold ${size}px 'Arial Rounded MT Bold', ArialRounded, system-ui, sans-serif`,
  (size) => `${size}px system-ui, -apple-system, sans-serif`,
];
const DRAW_WORDS = 22;
const FIELD_WORDS = 6;
const DRAW_RECT = 1;
const DRAW_TEXT = 2;
const DRAW_BORDER = 3;
const FLAG_STRIKE = 1;
const FLAG_DASHED = 2;
const CURSORS = ["default", "pointer", "grabbing"];
const STORE_KEY = "minipomo";
const APP_KEYS = { Escape: 0, Enter: 1, " ": 2 };
const DASH = [6, 4];
const CHIME_HZ = [523, 659];
const CHIME_GAP_SEC = 0.25;
const CHIME_FADE_SEC = 1.2;
const CHIME_RISE_SEC = 0.02;
const CHIME_GAIN = 0.12;
const CHIME_FLOOR = 0.0001;

const canvas = document.getElementById("app");
const ctx = canvas.getContext("2d");
const inputs = [document.getElementById("title"), document.getElementById("note")];
const versions = [0, 0];
const encoder = new TextEncoder();
const decoder = new TextDecoder();
const pointer = { x: -1, y: -1, down: 0, up: 0 };
let app, memory, audio;
let storageError = false;
let storageWritable = false;

const now = () => Date.now();
const text = (ptr, len) => decoder.decode(new Uint8Array(memory.buffer, ptr, len));
const cstring = (ptr) => text(ptr, new Uint8Array(memory.buffer, ptr).indexOf(0));
const rgba = (f, o) => `rgba(${f[o]},${f[o + 1]},${f[o + 2]},${f[o + 3] / 255})`;

function writeIo(s) {
  const io = new Uint8Array(memory.buffer, app.app_io(), app.app_io_size());
  return encoder.encodeInto(s, io).written;
}

function chime() {
  if (!audio) return;
  CHIME_HZ.forEach((hz, i) => {
    const t = audio.currentTime + i * CHIME_GAP_SEC;
    const osc = audio.createOscillator();
    const gain = audio.createGain();
    osc.frequency.value = hz;
    gain.gain.setValueAtTime(CHIME_FLOOR, t);
    gain.gain.linearRampToValueAtTime(CHIME_GAIN, t + CHIME_RISE_SEC);
    gain.gain.exponentialRampToValueAtTime(CHIME_FLOOR, t + CHIME_FADE_SEC);
    osc.connect(gain).connect(audio.destination);
    osc.start(t);
    osc.stop(t + CHIME_FADE_SEC);
  });
}

const env = {
  measure(ptr, len, font, size) {
    ctx.font = FONTS[font](size);
    return ctx.measureText(text(ptr, len)).width;
  },
  notify(ptr) {
    if (window.Notification && Notification.permission === "granted") {
      new Notification("MiniPomo", { body: cstring(ptr) });
    }
    chime();
  },
  log(ptr, len) {
    console.warn(text(ptr, len));
  },
};

function paintBorder(f, o, x, y, w, h) {
  const [l, r, t, b] = [f[o + 12], f[o + 13], f[o + 14], f[o + 15]];
  if (l === r && r === t && t === b) {
    ctx.strokeStyle = ctx.fillStyle;
    ctx.lineWidth = l;
    ctx.setLineDash(new Int32Array(memory.buffer)[o + 19] & FLAG_DASHED ? DASH : []);
    ctx.beginPath();
    ctx.roundRect(x + l / 2, y + l / 2, w - l, h - l, f[o + 8]);
    ctx.stroke();
    return;
  }
  ctx.fillRect(x, y, l, h);
  ctx.fillRect(x + w - r, y, r, h);
  ctx.fillRect(x, y, w, t);
  ctx.fillRect(x, y + h - b, w, b);
}

function paint(n, w, h) {
  const f = new Float32Array(memory.buffer);
  const i32 = new Int32Array(memory.buffer);
  const base = app.app_draws() / 4;
  ctx.clearRect(0, 0, w, h);
  ctx.textBaseline = "middle";
  for (let k = 0; k < n; k++) {
    const o = base + k * DRAW_WORDS;
    const [x, y, bw, bh] = [f[o], f[o + 1], f[o + 2], f[o + 3]];
    ctx.fillStyle = rgba(f, o + 4);
    const kind = i32[o + 17];
    if (kind === DRAW_RECT) {
      ctx.beginPath();
      ctx.roundRect(x, y, bw, bh, [f[o + 8], f[o + 9], f[o + 11], f[o + 10]]);
      ctx.fill();
    } else if (kind === DRAW_BORDER) {
      paintBorder(f, o, x, y, bw, bh);
    } else if (kind === DRAW_TEXT) {
      const s = text(i32[o + 20], i32[o + 21]);
      ctx.font = FONTS[i32[o + 18]](f[o + 16]);
      ctx.fillText(s, x, y + bh / 2);
      if (i32[o + 19] & FLAG_STRIKE) {
        ctx.fillRect(x, y + bh / 2, ctx.measureText(s).width, 1);
      }
    }
  }
}

function placeFields() {
  const f = new Float32Array(memory.buffer);
  const i32 = new Int32Array(memory.buffer);
  const base = app.app_fields() / 4;
  inputs.forEach((el, k) => {
    const o = base + k * FIELD_WORDS;
    if (!f[o + 2]) {
      el.style.display = "none";
      return;
    }
    Object.assign(el.style, {
      display: "block",
      left: `${f[o]}px`,
      top: `${f[o + 1]}px`,
      width: `${f[o + 2]}px`,
      height: `${f[o + 3]}px`,
    });
    if (versions[k] !== i32[o + 4]) {
      versions[k] = i32[o + 4];
      el.value = cstring(i32[o + 5]);
      if (k === 0) el.focus();
    }
  });
}

function frame() {
  const w = document.documentElement.clientWidth, dpr = devicePixelRatio;
  const n = app.app_frame(w, innerHeight, now(), new Date().getTimezoneOffset(),
    pointer.x, pointer.y, pointer.down);
  if (pointer.up) pointer.down = pointer.up = 0;

  const h = Math.ceil(app.app_height());
  if (canvas.width !== w * dpr || canvas.height !== h * dpr) {
    canvas.width = w * dpr;
    canvas.height = h * dpr;
    canvas.style.width = `${w}px`;
    canvas.style.height = `${h}px`;
  }
  ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  paint(n, w, h);
  placeFields();
  canvas.style.cursor = CURSORS[app.app_cursor()];

  const title = cstring(app.app_title());
  if (document.title !== title) document.title = title;
  const len = storageWritable ? app.app_save() : -1;
  if (len >= 0) {
    try {
      localStorage.setItem(STORE_KEY, text(app.app_io(), len));
      app.app_saved();
      storageError = false;
    } catch (error) {
      if (!storageError) console.error("Cannot save MiniPomo state", error);
      storageError = true;
    }
  }
}

function loop() {
  frame();
  requestAnimationFrame(loop);
}

function move(e) {
  pointer.x = e.offsetX;
  pointer.y = e.offsetY;
}

function enableNotifications() {
  audio ??= new AudioContext();
  if (window.Notification && Notification.permission === "default") {
    Notification.requestPermission();
  }
}

canvas.addEventListener("pointermove", move);
canvas.addEventListener("pointerdown", (e) => {
  move(e);
  pointer.down = 1;
  enableNotifications();
});
addEventListener("pointerup", () => {
  pointer.up = 1;
});
canvas.addEventListener("pointerleave", () => {
  if (!pointer.down) pointer.x = pointer.y = -1;
});
inputs.forEach((el, k) => {
  el.addEventListener("input", () => app.app_input(k, writeIo(el.value)));
});
addEventListener("keydown", (e) => {
  if (!app || e.isComposing || e.keyCode === 229 || e.ctrlKey || e.metaKey || e.altKey) return;
  const field = inputs.indexOf(e.target);
  if (field >= 0) {
    if (e.key !== "Escape" && !(e.key === "Enter" && field === 0)) return;
  } else if (e.target !== document.body && e.target !== canvas) {
    return;
  }
  if (!Object.hasOwn(APP_KEYS, e.key)) return;
  e.preventDefault();
  if (e.repeat) return;
  enableNotifications();
  app.app_key(APP_KEYS[e.key], now());
  frame();
});

const { instance } = await WebAssembly.instantiateStreaming(fetch("app.wasm"), { env });
app = instance.exports;
memory = app.memory;
if (app.app_init() !== 0) throw new Error("app_init failed");
try {
  const saved = localStorage.getItem(STORE_KEY);
  if (saved && (encoder.encode(saved).length > app.app_io_size() || app.app_load(writeIo(saved)) !== 0)) {
    console.error("Cannot load MiniPomo state; saving is disabled");
  } else {
    storageWritable = true;
  }
} catch (error) {
  console.error("Cannot read MiniPomo state; saving is disabled", error);
}
loop();

const hiddenTabTimer = new Worker(URL.createObjectURL(new Blob(
  ["setInterval(() => postMessage(0), 1000);"], { type: "text/javascript" })));
hiddenTabTimer.onmessage = () => document.hidden && frame();
