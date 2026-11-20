import {readFileSync} from "node:fs";
import {resolve} from "node:path";
import {pathToFileURL} from "node:url";
const dir=process.env.GUTTS_DIST;
if(!dir)throw new Error("GUTTS_DIST required");
const {defineApi}=await import(pathToFileURL(resolve(dir,"api/define-api.js")).href);
const rows=readFileSync(new URL("../testdata/upstream-golden/api-definition.tsv",import.meta.url),"utf8")
  .split(/\r?\n/).filter(x=>x&&!x.startsWith("#"));
let verified=0;
for(const row of rows){
  const sep=row.indexOf("\t");
  if(sep<0)throw new Error("Malformed API definition fixture");
  const input=JSON.parse(row.slice(0,sep)),expected=row.slice(sep+1);
  const actual=JSON.stringify(defineApi(input));
  if(actual!==expected)throw new Error(JSON.stringify({id:input.id,expected,actual}));
  verified++;
}
if(verified<8)throw new Error("Missing upstream API fixtures");
console.log("Pinned GutTS defineApi fixtures passed: "+verified);
