import {readFileSync} from "node:fs";
import {resolve} from "node:path";
import {pathToFileURL} from "node:url";
const dist=process.env.GUTTS_DIST;
if(!dist)throw Error("GUTTS_DIST required");
const source=readFileSync(resolve(dist,"../src/storage/s3.ts"),"utf8");
const match=source.match(/export\s*\{([\s\S]*?)\}\s*from\s*["']@aws-sdk\/client-s3["']/);
if(!match)throw Error("Pinned GutTS no longer reexports external AWS S3 SDK");
const names=match[1].split(",").map(x=>x.trim()).filter(Boolean);
const lines=readFileSync(new URL("../testdata/upstream-golden/s3-commands.tsv",import.meta.url),"utf8")
 .split(/\r?\n/).filter(x=>x&&!x.startsWith("#"));
const expected=["S3Client",...lines.map((l,i)=>{
 const [name,index]=l.split("\t");
 if(Number(index)!==i)throw Error("Native S3 command ordering corrupted");
 return name;
})];
if(expected.length!==8||JSON.stringify(names)!==JSON.stringify(expected))
 throw Error("Native S3 command list differs from pinned upstream external SDK exports");
const actual=await import(pathToFileURL(resolve(dist,"storage/s3.js")).href);
for(const name of expected)
 if(typeof actual[name]!=="function")
  throw Error("Pinned GutTS AWS SDK reexport missing actual constructor "+name);
console.log("Pinned GutTS S3 external AWS SDK reexports verified: "+expected.length+
 " names; native C uses explicit host-backed binary client (not JS SDK/AWS E2E)");
