import {readFileSync} from "node:fs";
import {resolve} from "node:path";
import {pathToFileURL} from "node:url";
const dir=process.env.GUTTS_DIST;
if(!dir)throw new Error("GUTTS_DIST required");
const {guttsApiToFodNode}=await import(pathToFileURL(resolve(dir,"api/fod-node-provider.js")).href);
const rows=readFileSync(new URL("../testdata/upstream-golden/fod.tsv",import.meta.url),"utf8")
  .split(/\r?\n/).filter(x=>x&&!x.startsWith("#"));
let count=0;
for(const line of rows){
  const i=line.indexOf("\t");
  if(i<0)throw new Error("Bad FOD golden row");
  const api=JSON.parse(line.slice(0,i)),expected=line.slice(i+1);
  const actual=JSON.stringify(guttsApiToFodNode(api));
  if(actual!==expected)throw new Error(JSON.stringify({id:api.id,expected,actual}));
  count++;
}
if(count<8)throw new Error("Missing FOD node cases");
console.log("Pinned GutTS FOD metadata fixtures passed: "+count);
