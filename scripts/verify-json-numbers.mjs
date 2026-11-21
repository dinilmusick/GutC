import {readFileSync} from "node:fs";
import {resolve} from "node:path";
import {pathToFileURL} from "node:url";
const dist=process.env.GUTTS_DIST;
if(!dist)throw Error("GUTTS_DIST required");
const {stringifyJson}=await import(pathToFileURL(resolve(dist,"json/json-concepts.js")).href);
const lines=readFileSync(new URL("../testdata/upstream-golden/json-numbers.tsv",import.meta.url),"utf8")
 .split(/\r?\n/).filter(s=>s&&!s.startsWith("#"));
let checked=0;
for(const line of lines){
 const parts=line.split("\t");
 if(parts.length!==2)throw Error("Malformed JSON number fixture "+line);
 const [literal,expected]=parts;
 const n=Number(literal);
 if(!Number.isFinite(n))throw Error("Non-finite fixture "+literal);
 const actual=stringifyJson(n);
 if(actual!==expected)throw Error(JSON.stringify({literal,actual,expected}));
 checked++;
}
if(checked!==351)throw Error("Expected 351 pinned JS numeric cases");
if(stringifyJson(NaN)!=="null"||stringifyJson(Infinity)!=="null"||
   stringifyJson(-Infinity)!=="null"||stringifyJson(-0)!=="0")
 throw Error("Pinned non-finite or negative-zero numeric JSON behavior changed");
console.log("Pinned GutTS shortest-roundtrip JS numeric JSON fixtures passed: "+checked);
