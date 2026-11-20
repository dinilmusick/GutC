import {readFileSync} from "node:fs";
import {resolve} from "node:path";
import {pathToFileURL} from "node:url";
const dir=process.env.GUTTS_DIST;
if(!dir)throw new Error("GUTTS_DIST required");
const semantic=await import(pathToFileURL(resolve(dir,"semantic-input/index.js")).href);
const rows=readFileSync(new URL("../testdata/upstream-golden/semantic.tsv",import.meta.url),"utf8")
  .split(/\r?\n/).filter(x=>x&&!x.startsWith("#"));
let checked=0;
for(const row of rows){
  const parts=row.split("\t");
  if(parts.length!==3)throw new Error("Malformed semantic fixture");
  const [raw,schemaRaw,expected]=parts;
  const input=JSON.parse(raw),schema=JSON.parse(schemaRaw);
  const output=semantic.resolveDeterministicSemanticInput(input,schema);
  const actual=output===undefined?"_":JSON.stringify(output);
  if(actual!==expected)throw new Error(JSON.stringify({input,schema,expected,actual}));
  checked++;
}
if(checked<30)throw new Error("Insufficient deterministic golden fixtures");
console.log("Pinned GutTS deterministic semantic fixtures passed: "+checked);
