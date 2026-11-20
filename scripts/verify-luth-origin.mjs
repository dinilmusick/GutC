import {readFileSync} from "node:fs";
import {resolve} from "node:path";
import {pathToFileURL} from "node:url";
const dir=process.env.GUTTS_DIST;
if(!dir)throw new Error("GUTTS_DIST required");
const {resolveClientOrigin,resolveClientEnvironment,DEFAULT_LUTH_URL}=
  await import(pathToFileURL(resolve(dir,"auth/luthAuthClient.js")).href);
const lines=readFileSync(new URL("../testdata/upstream-golden/luth-origin.tsv",import.meta.url),"utf8")
  .split(/\r?\n/).filter(x=>x&&!x.startsWith("#"));
let checked=0;
for(const row of lines){
  const parts=row.split("\t");
  if(parts.length!==4)throw new Error("Malformed origin fixture");
  const [explicit,locationOrigin,wanted,env]=parts.map(JSON.parse);
  globalThis.location={origin:locationOrigin||""};
  const actual=resolveClientOrigin(explicit===null?undefined:explicit);
  const classified=resolveClientEnvironment(actual);
  if(actual!==wanted||classified!==env)
    throw new Error(JSON.stringify({explicit,locationOrigin,wanted,actual,env,classified}));
  checked++;
}
if(checked<25)throw new Error("Insufficient Luth URL corpus");
if(DEFAULT_LUTH_URL!=="https://luth.blueprint-ninjastudios.workers.dev")
  throw new Error("Default Luth URL differs");
console.log("Pinned GutTS Luth origin and environment cases passed: "+checked);
