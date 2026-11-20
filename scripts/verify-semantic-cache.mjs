import {readFileSync} from "node:fs";
import {resolve} from "node:path";
import {pathToFileURL} from "node:url";
const dist=process.env.GUTTS_DIST;
if(!dist)throw new Error("GUTTS_DIST required");
const {SemanticInputCache}=await import(pathToFileURL(resolve(dist,"semantic-input/index.js")).href);
const rows=readFileSync(new URL("../testdata/upstream-golden/semantic-cache.tsv",import.meta.url),"utf8")
 .split(/\r?\n/).filter(x=>x&&!x.startsWith("#"));
let cache=new SemanticInputCache(),checked=0,readCount=0;
for(const line of rows){
 const cols=line.split("\t");
 if(cols.length!==6)throw new Error("Malformed cache fixture");
 const [op,keyJSON,valueJSON,maxJSON,want,sizeJSON]=cols;
 const key=JSON.parse(keyJSON),value=JSON.parse(valueJSON),
   max=JSON.parse(maxJSON),expectedSize=JSON.parse(sizeJSON);
 let actual="_";
 if(op==="new")cache=new SemanticInputCache(max);
 else if(op==="set")cache.set(key,value);
 else if(op==="get"){
   const output=cache.get(key);
   actual=output===undefined?"_":JSON.stringify(output);
   readCount++;
 }else if(op==="clear")cache.clear();
 else throw new Error("Unknown cache op "+op);
 if(actual!==want||cache.size!==expectedSize)
   throw new Error(JSON.stringify({op,key,actual,want,size:cache.size,expectedSize}));
 checked++;
}
if(checked<55||readCount<20)throw new Error("Insufficient LRU fixture coverage");
console.log("Pinned GutTS semantic-input LRU cache fixtures passed: "+checked+" operations, "+readCount+" gets");
