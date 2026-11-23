const canvas = document.querySelector("#canvas");
const ctx = canvas.getContext("2d");
const PLAYER_HEIGHT = 20;
const state = {
  level: {
    playerPosition: { x: -350, y: 50, z: 200 }, playerSize: { x: 80, y: 80 },
    cubes: [{ position: { x: -700, y: 0, z: 0 }, size: { x: 1100, y: 600, z: 200 } }],
    spikes: [], movingCubes: [],
    finish: { position: { x: 1950, y: 0, z: 200 }, size: { x: 140, y: 600, z: 500 } },
    instructions: [{ text: "Reach the finish.", textPosition: { x: 220, y: 105 }, textSize: 32, buttonText: "Start", buttonPosition: { x: 640, y: 555 }, buttonSize: { x: 180, y: 54 } }], starTimes: [15, 30, 60]
  }, selected: { type: "cube", index: 0 }, yaw: -0.65, pitch: 0.45, zoom: 0.42, panX: 0, panY: 0,
  fileName: "level-editor.level",
  playerView: {},        // editor-only view flags for the player start (it has no object of its own)
  playMotion: true,      // preview moving platforms as they will move in the game
  animTime: 0            // seconds since the level "started" (drives moving platforms)
};
const NOUN = { cube: "platform", movingCube: "moving platform", spike: "spike", finish: "finish", player: "player start" };
// [top, side] base colours per object type
const colors = { cube: ["#477da8", "#2c5879"], movingCube: ["#9a70d6", "#624298"], spike: ["#e15a5a", "#963333"], finish: ["#e6b941", "#9d7218"], player: ["#e16e74", "#973e49"] };
const num = value => Number.isFinite(Number(value)) ? Number(value) : 0;
const escapeText = text => `"${String(text).replace(/\\/g, "\\\\").replace(/"/g, '\\"').replace(/\r?\n/g, "\\n")}"`;
const escapeHtml = text => String(text).replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;").replace(/"/g, "&quot;");
const shade = (hex, f) => "#" + [1, 3, 5].map(i =>
  Math.round(Math.min(255, parseInt(hex.slice(i, i + 2), 16) * f)).toString(16).padStart(2, "0")).join("");

/* ---------------------------------------------------------------- camera */

function resize() {
  const dpr = devicePixelRatio || 1;
  canvas.width = canvas.clientWidth * dpr; canvas.height = canvas.clientHeight * dpr;
  ctx.setTransform(dpr, 0, 0, dpr, 0, 0); draw();
}
new ResizeObserver(resize).observe(canvas);

function center() { return { x: canvas.clientWidth / 2 + state.panX, y: canvas.clientHeight / 2 + state.panY }; }

// Orthographic camera that sits ABOVE the ground and looks down by `pitch`.
//   right = ( cy, -sy,   0)   up = ( sp*sy, sp*cy, cp)   forward = ( sy*cp, cy*cp, -sp)
function viewDir() {
  const cy = Math.cos(state.yaw), sy = Math.sin(state.yaw), cp = Math.cos(state.pitch), sp = Math.sin(state.pitch);
  return [sy * cp, cy * cp, -sp];
}
function project(x, y, z) {
  const cy = Math.cos(state.yaw), sy = Math.sin(state.yaw), cp = Math.cos(state.pitch), sp = Math.sin(state.pitch);
  const rx = x * cy - y * sy, ry = x * sy + y * cy;
  const c = center();
  // depth grows with distance from the camera (farther = larger)
  return { x: c.x + rx * state.zoom, y: c.y - (z * cp + ry * sp) * state.zoom, depth: ry * cp - z * sp };
}

/* ------------------------------------------------------------- geometry */

// Axis-aligned bounds that stay valid even if the user types a negative size.
function bounds(box) {
  const p = [box.position.x, box.position.y, box.position.z], s = [box.size.x, box.size.y, box.size.z];
  return { min: p.map((v, i) => Math.min(v, v + s[i])), max: p.map((v, i) => Math.max(v, v + s[i])) };
}
function boxCorners(b) {
  const out = [];
  for (const x of [b.min[0], b.max[0]]) for (const y of [b.min[1], b.max[1]]) for (const z of [b.min[2], b.max[2]]) out.push([x, y, z]);
  return out;
}
// Only the faces that point towards the camera (back-face culling).
function visibleFaces(b, d) {
  const out = [];
  for (let a = 0; a < 3; a++) for (const sign of [-1, 1]) {
    if (sign * d[a] >= 0) continue;
    const u = (a + 1) % 3, v = (a + 2) % 3, k = sign > 0 ? b.max[a] : b.min[a];
    const points = [[b.min[u], b.min[v]], [b.max[u], b.min[v]], [b.max[u], b.max[v]], [b.min[u], b.max[v]]].map(([pu, pv]) => {
      const p = [0, 0, 0]; p[a] = k; p[u] = pu; p[v] = pv; return project(...p);
    });
    out.push({ axis: a, sign, points });
  }
  return out;
}
function faceColor(face, c) {
  if (face.axis === 2) return face.sign > 0 ? c[0] : shade(c[1], .6);
  return face.axis === 0 ? c[1] : shade(c[1], .8);
}

/* -------------------------------------------------------------- drawing */

function drawGrid() {
  ctx.strokeStyle = "#24313d"; ctx.lineWidth = 1;
  for (let x = -3000; x <= 3000; x += 200) { const a = project(x, -3000, 0), b = project(x, 3000, 0); ctx.beginPath(); ctx.moveTo(a.x, a.y); ctx.lineTo(b.x, b.y); ctx.stroke(); }
  for (let y = -3000; y <= 3000; y += 200) { const a = project(-3000, y, 0), b = project(3000, y, 0); ctx.beginPath(); ctx.moveTo(a.x, a.y); ctx.lineTo(b.x, b.y); ctx.stroke(); }
}
// Dashed wireframe shown only for a hidden object that is currently selected.
function drawGhost(b) {
  const pts = boxCorners(b).map(c => project(...c));   // index bits: x*4 + y*2 + z
  ctx.save(); ctx.setLineDash([6, 4]); ctx.strokeStyle = "#c9d7e4"; ctx.lineWidth = 1.5; ctx.beginPath();
  for (let i = 0; i < 8; i++) for (const bit of [1, 2, 4]) if (!(i & bit)) { const a = pts[i], c = pts[i | bit]; ctx.moveTo(a.x, a.y); ctx.lineTo(c.x, c.y); }
  ctx.stroke(); ctx.restore();
}
// The space a moving platform sweeps through is its box slid along the travel vector. That solid is
// convex, so its outline is just the edges of its faces. Every face normal is either a box axis or is
// perpendicular to a box edge and the travel vector. Returns each outline edge once as [[x,y,z],[x,y,z]].
// (Drawing both end boxes plus corner-to-corner connectors puts lines through the inside of the volume
// whenever the travel is diagonal.)
function sweptEdges(box) {
  const t = [box.travel.x, box.travel.y, box.travel.z];
  const key = p => p.map(v => Math.round(v * 1000)).join(",");
  const pts = [], seenPts = new Set();
  for (const c of boxCorners(bounds(box))) for (const k of [0, 1]) {
    const p = c.map((v, i) => v + t[i] * k);
    if (!seenPts.has(key(p))) { seenPts.add(key(p)); pts.push(p); }
  }
  const dot = (a, b) => a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
  const cross3 = (a, b) => [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]];
  const unit = a => { const l = Math.hypot(...a); return a.map(v => v / l); };

  const normals = [];
  for (let a = 0; a < 3; a++) for (const s of [-1, 1]) { const n = [0, 0, 0]; n[a] = s; normals.push(n); }
  for (let a = 0; a < 3; a++) {
    const e = [0, 0, 0]; e[a] = 1;
    const n = cross3(e, t);
    if (Math.hypot(...n) > 1e-9) { const u = unit(n); normals.push(u, u.map(v => -v)); }
  }

  const edges = [], seenEdges = new Set();
  for (const n of normals) {
    const d = pts.map(p => dot(n, p)), top = Math.max(...d);
    const face = pts.filter((_, i) => d[i] >= top - 1e-6);        // the vertices lying on this face
    if (face.length < 3) continue;
    // convex hull of the face inside its own plane (monotone chain), keeping only true corners
    const axis = [0, 0, 0]; axis[[0, 1, 2].sort((i, j) => Math.abs(n[i]) - Math.abs(n[j]))[0]] = 1;
    const u = unit(cross3(n, axis)), v = cross3(n, u);
    const q = face.map(p => ({ p, x: dot(u, p), y: dot(v, p) })).sort((a, b) => a.x - b.x || a.y - b.y);
    const cr = (a, b, c) => (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    const chain = list => { const h = []; for (const pt of list) { while (h.length >= 2 && cr(h[h.length - 2], h[h.length - 1], pt) <= 1e-6) h.pop(); h.push(pt); } h.pop(); return h; };
    const hull = chain(q).concat(chain([...q].reverse()));
    if (hull.length < 3) continue;
    for (let i = 0; i < hull.length; i++) {
      const a = hull[i].p, b = hull[(i + 1) % hull.length].p, ka = key(a), kb = key(b);
      const id = ka < kb ? ka + "|" + kb : kb + "|" + ka;
      if (!seenEdges.has(id)) { seenEdges.add(id); edges.push([a, b]); }
    }
  }
  return edges;
}
function drawMovingPath(box) {
  const start = bounds(box);
  const end = bounds({
    position: { x: box.position.x + box.travel.x, y: box.position.y + box.travel.y, z: box.position.z + box.travel.z },
    size: box.size
  });
  const allCorners = boxCorners(start).concat(boxCorners(end)).map(corner => project(...corner));

  // translucent fill: the on-screen silhouette (convex hull) of the swept volume
  ctx.save();
  ctx.globalAlpha = .12;
  ctx.fillStyle = "#b58cff";
  const cross = (a, b, c) => (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
  const sorted = [...allCorners].sort((a, b) => a.x === b.x ? a.y - b.y : a.x - b.x);
  const lower = [];
  for (const point of sorted) {
    while (lower.length >= 2 && cross(lower[lower.length - 2], lower[lower.length - 1], point) <= 0) lower.pop();
    lower.push(point);
  }
  const upper = [];
  for (let i = sorted.length - 1; i >= 0; i--) {
    const point = sorted[i];
    while (upper.length >= 2 && cross(upper[upper.length - 2], upper[upper.length - 1], point) <= 0) upper.pop();
    upper.push(point);
  }
  lower.pop();
  upper.pop();
  const hull = lower.concat(upper);
  if (hull.length >= 3) {
    ctx.beginPath();
    hull.forEach((point, i) => i ? ctx.lineTo(point.x, point.y) : ctx.moveTo(point.x, point.y));
    ctx.closePath();
    ctx.fill();
  }

  // dotted outline: only the edges of the swept volume itself
  ctx.globalAlpha = .7;
  ctx.setLineDash([6, 4]);
  ctx.strokeStyle = "#d4b8ff";
  ctx.lineWidth = 1.5;
  ctx.beginPath();
  for (const [a, b] of sweptEdges(box)) {
    const pa = project(...a), pb = project(...b);
    ctx.moveTo(pa.x, pa.y);
    ctx.lineTo(pb.x, pb.y);
  }
  ctx.stroke();
  ctx.restore();
}
/* ------------------------------------------------- moving platform motion */

// MOTION MODEL (the one place to change if the game moves platforms differently):
// a moving platform travels from its start position to start + travel at `speed` units per second,
// then back again, forever (ping-pong, constant speed). Returns 0..1 = how far along `travel` it is.
function movingFraction(box, t) {
  const len = Math.hypot(box.travel.x, box.travel.y, box.travel.z);
  const speed = Math.max(0, box.speed);
  if (!len || !speed) return 0;
  const m = (speed * t) % (2 * len);
  return (m <= len ? m : 2 * len - m) / len;
}
function movingBox(box, t) {
  const f = movingFraction(box, t);
  return { position: { x: box.position.x + box.travel.x * f, y: box.position.y + box.travel.y * f, z: box.position.z + box.travel.z * f }, size: box.size };
}
function animationNeeded() {
  return state.playMotion && state.level.movingCubes.some(c => c.speed && (c.travel.x || c.travel.y || c.travel.z));
}
let lastDraw = 0;
function tick(now) {
  requestAnimationFrame(tick);
  if (!animationNeeded()) { lastDraw = now; return; }
  const dt = now - lastDraw; if (dt < 33) return;      // ~30 fps: the software renderer redraws the whole scene
  state.animTime += Math.min(dt, 100) / 1000; lastDraw = now; draw();
}

function sceneObjects() {
  const l = state.level;
  const player = { position: { x: l.playerPosition.x, y: l.playerPosition.y, z: l.playerPosition.z }, size: { x: l.playerSize.x, y: 8, z: l.playerSize.y } };
  return l.cubes.map((box, index) => ({ box, type: "cube", index, label: `platform ${index + 1}` }))
    // `shown` is where the platform is drawn right now; `box` stays the saved start position
    .concat(l.movingCubes.map((box, index) => ({ box, type: "movingCube", index, label: `moving platform ${index + 1}`, shown: state.playMotion ? movingBox(box, state.animTime) : box })))
    .concat(l.spikes.map((box, index) => ({ box, type: "spike", index, label: `spike ${index + 1}` })))
    .concat([{ box: l.finish, type: "finish", index: 0, label: "finish" }, { box: player, type: "player", index: 0, label: "player" }])
    .map(o => ({ ...o, hidden: isHidden(o.type, o.index) }));
}
// ---- software z-buffer: exact visibility even when boxes overlap / intersect ----
const DEPTH_BIAS = 0.05;                       // coplanar faces: first drawn wins, no flicker
const pack = hex => (0xff000000 | (parseInt(hex.slice(5, 7), 16) << 16) | (parseInt(hex.slice(3, 5), 16) << 8) | parseInt(hex.slice(1, 3), 16)) >>> 0;
const EDGE = pack("#18222c"), EDGE_SELECTED = pack("#ffffff");
const layer = document.createElement("canvas"), layerCtx = layer.getContext("2d");
let zbuf = new Float32Array(0), idbuf = new Int32Array(0), img = null, lastObjs = [], rasterScale = 1;
let rx0, rx1, ry0, ry1;                        // dirty rectangle of the current frame

// Orthographic projection => depth is affine in screen space, so interpolate it barycentrically.
function fillTri(p0, p1, p2, id, W, H) {
  const area = (p1.x - p0.x) * (p2.y - p0.y) - (p2.x - p0.x) * (p1.y - p0.y);
  if (Math.abs(area) < 1e-9) return;
  const x0 = Math.max(0, Math.floor(Math.min(p0.x, p1.x, p2.x))), x1 = Math.min(W - 1, Math.ceil(Math.max(p0.x, p1.x, p2.x)));
  const y0 = Math.max(0, Math.floor(Math.min(p0.y, p1.y, p2.y))), y1 = Math.min(H - 1, Math.ceil(Math.max(p0.y, p1.y, p2.y)));
  if (x1 < x0 || y1 < y0) return;
  rx0 = Math.min(rx0, x0); rx1 = Math.max(rx1, x1); ry0 = Math.min(ry0, y0); ry1 = Math.max(ry1, y1);
  const inv = 1 / area, eps = -1e-4;
  for (let y = y0; y <= y1; y++) {
    const py = y + .5;
    for (let x = x0; x <= x1; x++) {
      const px = x + .5;
      const w0 = ((p1.x - px) * (p2.y - py) - (p2.x - px) * (p1.y - py)) * inv;
      const w1 = ((p2.x - px) * (p0.y - py) - (p0.x - px) * (p2.y - py)) * inv;
      const w2 = 1 - w0 - w1;
      if (w0 < eps || w1 < eps || w2 < eps) continue;
      const z = w0 * p0.depth + w1 * p1.depth + w2 * p2.depth, i = y * W + x;
      if (z < zbuf[i] - DEPTH_BIAS) { zbuf[i] = z; idbuf[i] = id; }
    }
  }
}
// An edge pixel is the nearer pixel next to a different face (or the background).
function isEdge(i, x, y, id, z, r, W, H) {
  let j;
  if (x >= r && idbuf[j = i - r] !== id && zbuf[j] >= z) return true;
  if (x + r < W && idbuf[j = i + r] !== id && zbuf[j] >= z) return true;
  if (y >= r && idbuf[j = i - r * W] !== id && zbuf[j] >= z) return true;
  if (y + r < H && idbuf[j = i + r * W] !== id && zbuf[j] >= z) return true;
  return false;
}

function draw() {
  const w = canvas.clientWidth, h = canvas.clientHeight, s = state.selected, d = viewDir();
  ctx.clearRect(0, 0, w, h); ctx.fillStyle = "#0c1118"; ctx.fillRect(0, 0, w, h);
  drawGrid();
  const W = canvas.width, H = canvas.height;
  if (!W || !H) return;
  if (zbuf.length !== W * H) { zbuf = new Float32Array(W * H); idbuf = new Int32Array(W * H); img = new ImageData(W, H); layer.width = W; layer.height = H; }
  zbuf.fill(Infinity); idbuf.fill(-1);
  const sc = rasterScale = W / w;
  const objs = lastObjs = sceneObjects().map(o => ({ ...o, b: bounds(o.shown || o.box) }));
  const palette = [];
  rx0 = W; rx1 = -1; ry0 = H; ry1 = -1;
  objs.forEach((o, oi) => {
    if (o.hidden) return;                      // hidden objects are not drawn and cannot be clicked in the scene
    for (const f of visibleFaces(o.b, d)) {
      const id = oi * 6 + f.axis * 2 + (f.sign > 0 ? 1 : 0);
      palette[id] = pack(faceColor(f, colors[o.type]));
      const p = f.points.map(q => ({ x: q.x * sc, y: q.y * sc, depth: q.depth }));
      fillTri(p[0], p[1], p[2], id, W, H); fillTri(p[0], p[2], p[3], id, W, H);
    }
  });
  const selIndex = objs.findIndex(o => s.type === o.type && s.index === o.index);
  const px = new Uint32Array(img.data.buffer); px.fill(0);
  const r = Math.max(1, Math.round(sc));
  for (let y = ry0; y <= ry1; y++) for (let x = rx0; x <= rx1; x++) {
    const i = y * W + x, id = idbuf[i];
    if (id < 0) continue;
    const isSel = ((id / 6) | 0) === selIndex;
    px[i] = isEdge(i, x, y, id, zbuf[i], isSel ? r * 2 : r, W, H) ? (isSel ? EDGE_SELECTED : EDGE) : palette[id];
  }
  layerCtx.putImageData(img, 0, 0);
  ctx.drawImage(layer, 0, 0, w, h);
  objs.filter(o => o.type === "movingCube" && !o.hidden).forEach(o => drawMovingPath(o.box));
  if (selIndex >= 0) {
    const sel = objs[selIndex], text = sel.hidden ? `${sel.label} (hidden)` : sel.label;
    if (sel.hidden) drawGhost(sel.b);
    const p = project((sel.b.min[0] + sel.b.max[0]) / 2, (sel.b.min[1] + sel.b.max[1]) / 2, sel.b.max[2]);
    ctx.font = "12px Segoe UI"; ctx.textAlign = "center"; ctx.lineWidth = 3; ctx.strokeStyle = "#0c1118";
    ctx.strokeText(text, p.x, p.y - 10); ctx.fillStyle = "#fff"; ctx.fillText(text, p.x, p.y - 10);
    ctx.textAlign = "start";
  }
}
// Picking reads the same id buffer that was rendered, so it always matches what is on screen.
function pickAt(cx, cy) {
  const W = canvas.width, x = Math.floor(cx * rasterScale), y = Math.floor(cy * rasterScale);
  if (idbuf.length !== W * canvas.height || x < 0 || y < 0 || x >= W || y >= canvas.height) return null;
  const id = idbuf[y * W + x];
  return id < 0 ? null : lastObjs[(id / 6) | 0];
}

/* ------------------------------------------------------------------ UI */

function normalizeSelection() {
  const s = state.selected, n = state.level.cubes.length;
  if (s.type === "cube") state.selected = n ? { type: "cube", index: Math.min(s.index, n - 1) } : { type: "finish", index: 0 };
  else if (s.type === "movingCube") state.selected = state.level.movingCubes.length
    ? { type: "movingCube", index: Math.min(s.index, state.level.movingCubes.length - 1) } : { type: "finish", index: 0 };
  else if (s.type === "spike") state.selected = state.level.spikes.length
    ? { type: "spike", index: Math.min(s.index, state.level.spikes.length - 1) } : { type: "finish", index: 0 };
}
function getSelected() {
  const s = state.selected, l = state.level;
  if (s.type === "cube") return l.cubes[s.index];
  if (s.type === "movingCube") return l.movingCubes[s.index];
  if (s.type === "spike") return l.spikes[s.index];
  if (s.type === "finish") return l.finish;
  return { position: l.playerPosition, size: { x: l.playerSize.x, y: l.playerSize.y, z: PLAYER_HEIGHT } };
}
function refresh() { normalizeSelection(); syncSettings(); renderList(); renderFields(); renderInstructions(); draw(); }
function renderList() {
  const list = document.querySelector("#objectList"); list.innerHTML = "";
  state.level.cubes.forEach((_, i) => addListItem(list, `Platform ${i + 1}`, "cube", i));
  state.level.movingCubes.forEach((_, i) => addListItem(list, `Moving platform ${i + 1}`, "movingCube", i));
  state.level.spikes.forEach((_, i) => addListItem(list, `Spike ${i + 1}`, "spike", i));
  addListItem(list, "Finish", "finish", 0); addListItem(list, "Player start", "player", 0);
}
// Visibility is an editor-only view setting that works for every object type. The flag lives on the object
// itself (the player start has no object of its own, so it uses state.playerView) and is never written to the .level file.
function viewHolder(type, index) {
  const l = state.level;
  if (type === "cube") return l.cubes[index];
  if (type === "movingCube") return l.movingCubes[index];
  if (type === "spike") return l.spikes[index];
  if (type === "finish") return l.finish;
  return state.playerView;
}
function isHidden(type, index) { const h = viewHolder(type, index); return !!(h && h.hidden); }
function toggleHidden(type, index) {
  const h = viewHolder(type, index); if (!h) return;
  h.hidden = !h.hidden; refresh();
}
function showAll() {
  [...state.level.cubes, ...state.level.movingCubes, ...state.level.spikes, state.level.finish].forEach(o => delete o.hidden);
  state.playerView = {}; refresh();
}
function addListItem(list, text, type, index) {
  const hidden = isHidden(type, index);
  const el = document.createElement("div");
  el.className = `object ${type} ${state.selected.type === type && state.selected.index === index ? "selected" : ""} ${hidden ? "hidden-object" : ""}`;
  const label = document.createElement("span"); label.textContent = text; el.appendChild(label);
  const eye = document.createElement("button"); eye.className = "eye"; eye.textContent = hidden ? "Show" : "Hide";
  eye.title = hidden ? `Show this ${NOUN[type]}` : `Hide this ${NOUN[type]} (editor only)`;
  eye.onclick = ev => { ev.stopPropagation(); toggleHidden(type, index) };
  el.appendChild(eye);
  el.onclick = () => { state.selected = { type, index }; refresh() }; list.appendChild(el);
}
function field(label, path, value, min) { return `<label>${label}<input data-object-path="${path}" type="number" value="${value}"${min !== undefined ? ` min="${min}"` : ""}></label>`; }
function instructionField(label, instruction, path, value, min) {
  return `<label>${label}<input data-instruction-path="${path}" data-instruction-index="${instruction.index}" type="number" value="${value}"${min !== undefined ? ` min="${min}"` : ""}></label>`;
}
function deleteSelected() {
  const sel = state.selected;
  if (sel.type === "cube") state.level.cubes.splice(sel.index, 1);
  else if (sel.type === "movingCube") state.level.movingCubes.splice(sel.index, 1);
  else if (sel.type === "spike") state.level.spikes.splice(sel.index, 1);
  else return;
  refresh();
}
function renderFields() {
  const box = getSelected(), holder = document.querySelector("#transformFields"), name = document.querySelector("#selectionName");
  name.textContent = state.selected.type === "cube" ? `Platform ${state.selected.index + 1}` :
    state.selected.type === "movingCube" ? `Moving platform ${state.selected.index + 1}` :
    state.selected.type === "spike" ? `Spike ${state.selected.index + 1}` :
    state.selected.type === "finish" ? "Finish" : "Player start";
  holder.innerHTML = `<div class="transform"><span class="transform-title">Position</span>${field("X", "position.x", box.position.x)}${field("Y", "position.y", box.position.y)}${field("Z", "position.z", box.position.z)}</div>`;
  if (state.selected.type === "player") holder.innerHTML += `<div class="transform"><span class="transform-title">Size</span>${field("Width", "size.x", box.size.x)}${field("Height", "size.y", box.size.y)}</div>`;
  else holder.innerHTML += `<div class="transform"><span class="transform-title">Size</span>${field("Width", "size.x", box.size.x, 1)}${field("Height", "size.y", box.size.y, 1)}${field("Depth", "size.z", box.size.z, 1)}</div>`;
  if (state.selected.type === "movingCube") {
    holder.innerHTML += `<div class="transform"><span class="transform-title">Travel</span>${field("X", "travel.x", box.travel.x)}${field("Y", "travel.y", box.travel.y)}${field("Z", "travel.z", box.travel.z)}${field("Speed", "speed", box.speed, 0)}</div>`;
  }
  const sel = state.selected;
  holder.innerHTML += `<button class="wide" id="visibilityButton">${isHidden(sel.type, sel.index) ? "Show" : "Hide"} ${NOUN[sel.type]} (H)</button>`;
  if (sel.type === "cube" || sel.type === "movingCube" || sel.type === "spike") {
    holder.innerHTML += `<button class="delete" id="deleteButton">Delete ${NOUN[sel.type]}</button>`;
  }
  holder.querySelectorAll("input").forEach(input => input.onchange = () => {
    const value = num(input.value);
    if (state.selected.type === "player" && input.dataset.objectPath.startsWith("size.")) {
      state.level.playerSize[input.dataset.objectPath.split(".")[1]] = value;
    } else if (state.selected.type !== "player") setPath(box, input.dataset.objectPath, value);
    refresh();
  });
  const del = document.querySelector("#deleteButton"); if (del) del.onclick = deleteSelected;
  const vis = document.querySelector("#visibilityButton"); if (vis) vis.onclick = () => toggleHidden(state.selected.type, state.selected.index);
}
function setPath(object, path, value) {
  const parts = path.split(".");
  if (parts.length === 1) object[parts[0]] = value;
  else object[parts[0]][parts[1]] = value;
}
function decodeLevelString(value) {
  let decoded = "";
  for (let i = 0; i < value.length; i++) {
    if (value[i] === "\\" && i + 1 < value.length) {
      const escaped = value[++i];
      decoded += escaped === "n" ? "\n" : escaped === "r" ? "\r" : escaped === "t" ? "\t" : escaped;
    } else {
      decoded += value[i];
    }
  }
  return decoded;
}
function parseInstructionLine(line) {
  const values = [], pattern = /"((?:\\.|[^"])*)"|(\S+)/g;
  let match;
  while ((match = pattern.exec(line.slice("instruction".length).trim()))) values.push(match[1] !== undefined ? decodeLevelString(match[1]) : num(match[2]));
  if (values.length < 9) return null;
  return {
    text: values[0],
    textPosition: { x: values[1], y: values[2] },
    textSize: values[3],
    buttonText: values[4],
    buttonPosition: { x: values[5], y: values[6] },
    buttonSize: { x: values[7], y: values[8] }
  };
}
function renderInstructions() {
  const list = document.querySelector("#instructionList");
  list.innerHTML = "";
  state.level.instructions.forEach((instruction, index) => {
    const item = document.createElement("div");
    item.className = "instruction";
    item.innerHTML = `
      <label>Text<textarea data-instruction-text="${index}">${escapeHtml(instruction.text)}</textarea></label>
      <div class="instruction-grid">
        ${instructionField("Text X", { index }, "textPosition.x", instruction.textPosition.x)}
        ${instructionField("Text Y", { index }, "textPosition.y", instruction.textPosition.y)}
        ${instructionField("Text size", { index }, "textSize", instruction.textSize, 1)}
        <label>Button text<input data-instruction-button="${index}" value="${escapeHtml(instruction.buttonText)}"></label>
        ${instructionField("Button X", { index }, "buttonPosition.x", instruction.buttonPosition.x)}
        ${instructionField("Button Y", { index }, "buttonPosition.y", instruction.buttonPosition.y)}
        ${instructionField("Button width", { index }, "buttonSize.x", instruction.buttonSize.x, 1)}
        ${instructionField("Button height", { index }, "buttonSize.y", instruction.buttonSize.y, 1)}
      </div>
      <button class="delete" data-remove-instruction="${index}">Delete instruction</button>`;
    list.appendChild(item);
  });
  list.querySelectorAll("[data-instruction-text]").forEach(input => input.onchange = () => {
    state.level.instructions[Number(input.dataset.instructionText)].text = input.value;
  });
  list.querySelectorAll("[data-instruction-button]").forEach(input => input.onchange = () => {
    state.level.instructions[Number(input.dataset.instructionButton)].buttonText = input.value;
  });
  list.querySelectorAll("[data-instruction-path]").forEach(input => input.onchange = () => {
    const instruction = state.level.instructions[Number(input.dataset.instructionIndex)];
    setPath(instruction, input.dataset.instructionPath, num(input.value));
  });
  list.querySelectorAll("[data-remove-instruction]").forEach(button => button.onclick = () => {
    state.level.instructions.splice(Number(button.dataset.removeInstruction), 1);
    refresh();
  });
}
document.querySelectorAll("[data-path]").forEach(input => input.onchange = () => { setPath(state.level, input.dataset.path, num(input.value)); refresh() });
function syncSettings() { document.querySelectorAll("[data-path]").forEach(i => { const [a, b] = i.dataset.path.split("."); i.value = state.level[a][b] }); }
document.querySelector("#addCubeButton").onclick = () => { state.level.cubes.push({ position: { x: 0, y: 0, z: 0 }, size: { x: 400, y: 200, z: 200 } }); state.selected = { type: "cube", index: state.level.cubes.length - 1 }; refresh() };
document.querySelector("#addMovingCubeButton").onclick = () => { state.level.movingCubes.push({ position: { x: 0, y: 0, z: 0 }, size: { x: 400, y: 200, z: 200 }, travel: { x: 0, y: 0, z: 0 }, speed: 100 }); state.selected = { type: "movingCube", index: state.level.movingCubes.length - 1 }; refresh() };
document.querySelector("#addSpikeButton").onclick = () => { state.level.spikes.push({ position: { x: 0, y: 0, z: 0 }, size: { x: 100, y: 100, z: 100 } }); state.selected = { type: "spike", index: state.level.spikes.length - 1 }; refresh() };
document.querySelector("#addFinishButton").onclick = () => { state.level.finish = { position: { x: 0, y: 0, z: 0 }, size: { x: 300, y: 300, z: 300 } }; state.selected = { type: "finish", index: 0 }; refresh() };
document.querySelector("#addInstructionButton").onclick = () => {
  state.level.instructions.push({ text: "New instruction", textPosition: { x: 220, y: 105 }, textSize: 32, buttonText: "Start", buttonPosition: { x: 640, y: 555 }, buttonSize: { x: 180, y: 54 } });
  refresh();
};
document.querySelector("#resetViewButton").onclick = () => { state.yaw = -.65; state.pitch = .45; state.zoom = .42; state.panX = 0; state.panY = 0; draw() };
document.querySelector("#showAllButton").onclick = showAll;
const motionButton = document.querySelector("#motionButton");
function updateMotionButton() { motionButton.textContent = state.playMotion ? "Stop moving platforms (edit at start)" : "Play moving platforms"; }
motionButton.onclick = () => { state.playMotion = !state.playMotion; state.animTime = 0; updateMotionButton(); draw() };
addEventListener("keydown", e => {
  if (e.target instanceof HTMLInputElement) return;
  if (e.key === "Delete") deleteSelected();
  else if ((e.key === "h" || e.key === "H") && !e.ctrlKey && !e.metaKey && !e.altKey) toggleHidden(state.selected.type, state.selected.index);
});

/* --------------------------------------------------------- mouse input */

let drag = null;
canvas.onpointerdown = e => { drag = { x: e.clientX, y: e.clientY, yaw: state.yaw, pitch: state.pitch, panX: state.panX, panY: state.panY, button: e.button, moved: false }; canvas.setPointerCapture(e.pointerId) };
canvas.onpointermove = e => {
  if (!drag) return;
  const dx = e.clientX - drag.x, dy = e.clientY - drag.y;
  if (Math.hypot(dx, dy) > 4) drag.moved = true;
  if (e.shiftKey || drag.button === 1) { state.panX = drag.panX + dx; state.panY = drag.panY + dy }
  else { state.yaw = drag.yaw + dx * .008; state.pitch = Math.max(.05, Math.min(1.5, drag.pitch + dy * .008)) } // camera never goes below the ground
  draw();
};
canvas.onpointerup = e => {
  if (drag && !drag.moved && drag.button === 0) {   // plain click = pick whatever is on screen under the cursor
    const r = canvas.getBoundingClientRect(), o = pickAt(e.clientX - r.left, e.clientY - r.top);
    if (o) { state.selected = { type: o.type, index: o.index }; refresh(); }
  }
  drag = null;
};
canvas.onpointercancel = () => drag = null;
canvas.onwheel = e => { e.preventDefault(); state.zoom = Math.max(.08, Math.min(2, state.zoom * (e.deltaY < 0 ? 1.1 : .9))); draw() };

/* ----------------------------------------------------------- file I/O */

function parseLevel(text) {
  const l = { playerPosition: { x: 0, y: 0, z: 0 }, playerSize: { x: 80, y: 80 }, cubes: [], movingCubes: [], spikes: [], finish: { position: { x: 0, y: 0, z: 0 }, size: { x: 300, y: 300, z: 300 } }, instructions: [], starTimes: [15, 30, 60] };
  const tokens = text.split(/\r?\n/).filter(line => line.trim() && !line.trim().startsWith("#"));
  const re = /^(\w+)\s+(.*)$/;
  const nums = s => s.trim().split(/\s+/).map(num);
  for (const line of tokens) {
    const m = line.match(re); if (!m) continue; const n = m[1], v = m[2];
    if (n === "player_position") { const [x = 0, y = 0, z = 0] = nums(v); l.playerPosition = { x, y, z } }
    else if (n === "player_size") { const [x = 80, y = 80] = nums(v); l.playerSize = { x, y } }
    else if (n === "star_times") l.starTimes = nums(v);
    else if (n === "cube" || n === "finish") { const [px = 0, py = 0, pz = 0, sx = 0, sy = 0, sz = 0] = nums(v), b = { position: { x: px, y: py, z: pz }, size: { x: sx, y: sy, z: sz } }; n === "cube" ? l.cubes.push(b) : l.finish = b }
    else if (n === "spike") { const [px = 0, py = 0, pz = 0, sx = 0, sy = 0, sz = 0] = nums(v); l.spikes.push({ position: { x: px, y: py, z: pz }, size: { x: sx, y: sy, z: sz } }) }
    else if (n === "moving_cube") { const [px = 0, py = 0, pz = 0, sx = 0, sy = 0, sz = 0, tx = 0, ty = 0, tz = 0, speed = 0] = nums(v); l.movingCubes.push({ position: { x: px, y: py, z: pz }, size: { x: sx, y: sy, z: sz }, travel: { x: tx, y: ty, z: tz }, speed }) }
    else if (n === "instruction") {
      const instruction = parseInstructionLine(line);
      if (instruction) l.instructions.push(instruction);
    }
  } return l;
}
function serializeLevel(l) {
  const lines = ["# Created with Freshman Level Editor", `player_position ${l.playerPosition.x} ${l.playerPosition.y} ${l.playerPosition.z}`, `player_size ${l.playerSize.x} ${l.playerSize.y}`, `star_times ${l.starTimes.join(" ")}`];
  l.instructions.forEach(i => {
    lines.push(`instruction ${escapeText(i.text)} ${i.textPosition.x} ${i.textPosition.y} ${i.textSize} ${escapeText(i.buttonText)} ${i.buttonPosition.x} ${i.buttonPosition.y} ${i.buttonSize.x} ${i.buttonSize.y}`);
  });
  l.cubes.forEach(c => lines.push(`cube ${c.position.x} ${c.position.y} ${c.position.z} ${c.size.x} ${c.size.y} ${c.size.z}`));
  l.spikes.forEach(c => lines.push(`spike ${c.position.x} ${c.position.y} ${c.position.z} ${c.size.x} ${c.size.y} ${c.size.z}`));
  l.movingCubes.forEach(c => lines.push(`moving_cube ${c.position.x} ${c.position.y} ${c.position.z} ${c.size.x} ${c.size.y} ${c.size.z} ${c.travel.x} ${c.travel.y} ${c.travel.z} ${c.speed}`));
  const f = l.finish; lines.push(`finish ${f.position.x} ${f.position.y} ${f.position.z} ${f.size.x} ${f.size.y} ${f.size.z}`); return lines.join("\n") + "\n";
}
function validateLevel(l) {
  if (!l.instructions.length) return "Add at least one instruction: the game requires one.";
  if (l.starTimes.length !== 3 || l.starTimes[0] > l.starTimes[1] || l.starTimes[1] > l.starTimes[2]) {
    return "Star times must contain 3 ascending values.";
  }
  return "";
}
document.querySelector("#fileInput").onchange = e => { const file = e.target.files[0]; if (!file) return; const reader = new FileReader(); reader.onload = () => { state.level = parseLevel(reader.result); state.playerView = {}; state.animTime = 0; state.fileName = file.name; state.selected = state.level.cubes.length ? { type: "cube", index: 0 } : state.level.movingCubes.length ? { type: "movingCube", index: 0 } : state.level.spikes.length ? { type: "spike", index: 0 } : { type: "finish", index: 0 }; document.querySelector("#status").textContent = `Loaded ${file.name}`; refresh() }; reader.readAsText(file) };
document.querySelector("#saveButton").onclick = () => {
  const error = validateLevel(state.level);
  if (error) { document.querySelector("#status").textContent = error; return; }
  const blob = new Blob([serializeLevel(state.level)], { type: "text/plain" }), a = document.createElement("a");
  a.href = URL.createObjectURL(blob); a.download = state.fileName; a.click(); URL.revokeObjectURL(a.href);
  document.querySelector("#status").textContent = `Downloaded ${state.fileName}`;
};
updateMotionButton(); resize(); refresh(); requestAnimationFrame(tick);