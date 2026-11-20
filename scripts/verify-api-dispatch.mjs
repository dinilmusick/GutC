import {readFileSync} from "node:fs";
import {resolve} from "node:path";
import {pathToFileURL} from "node:url";
const dir=process.env.GUTTS_DIST;
if(!dir)throw new Error("GUTTS_DIST required");
const {executeGuttsApi}=await import(pathToFileURL(resolve(dir,"api/index.js")).href);
const rows=readFileSync(new URL("../testdata/upstream-golden/api-dispatch.tsv",import.meta.url),"utf8")
  .split(/\r?\n/).filter(x=>x&&!x.startsWith("#"));
let cases=0;
const covered=new Set();
for(const row of rows){
  const a=row.indexOf("\t"),b=row.lastIndexOf("\t");
  if(a<0||a===b)throw new Error("Malformed API dispatch golden row");
  const id=row.slice(0,a),input=JSON.parse(row.slice(a+1,b)),expected=row.slice(b+1);
  const result=await executeGuttsApi(id,input);
  const actual=result===undefined?"_":JSON.stringify(result);
  if(actual!==expected)
    throw new Error(JSON.stringify({id,input,expected,actual}));
  covered.add(id);
  cases++;
}
if(cases<45||covered.size!==33)throw new Error("Expected all 33 portable utility APIs, got "+cases+" cases across "+covered.size+" IDs");
console.log("Pinned GutTS executeGuttsApi utility fixtures passed: "+cases+" cases, "+covered.size+" APIs");
