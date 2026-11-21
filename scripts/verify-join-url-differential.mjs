// Real compiled, immutable GutTS joinUrl versus current C11 GutC executable.
// No copied JavaScript implementation or handwritten expected outputs.
import {execFileSync} from "node:child_process";
import {pathToFileURL} from "node:url";
import {resolve} from "node:path";
const dist=process.env.GUTTS_DIST;
const runner=process.env.GUTC_JOIN_URL_RUNNER;
if(!dist||!runner)throw Error("GUTTS_DIST and GUTC_JOIN_URL_RUNNER are required");
const {joinUrl}=await import(pathToFileURL(resolve(dist,"urls/url-operations.js")).href);
if(typeof joinUrl!=="function")throw Error("Pinned URL source does not export joinUrl");
const bases=["","/","///","https://example.com///","https://a.test/a//","abc","a//b","prefix?x=1","café/","😀///","foo#frag/"];
const samples=[[],[""],["/"],["///"],["alpha"],["/alpha/"],["//β//"],["a//b"],["?q=1"],["🙂"],["/","//"],["///","x/"],["/a/","//b//"],["","/q/",""],[" a ","/ b /"],["é","a+b"],["%2e","../"],["//","?x=1"]];
let count=0;
for(const base of bases)for(const parts of samples) {
  const actual=execFileSync(resolve(runner),[base,...parts],{maxBuffer:1048576});
  const expected=Buffer.from(joinUrl(base,...parts),"utf8");
  if(!actual.equals(expected))throw Error(JSON.stringify({base,parts,actual:actual.toString("hex"),expected:expected.toString("hex")}));
  count++;
}
if(count!==bases.length*samples.length)throw Error("Differential inventory changed");
console.log("GutC native versus compiled pinned GutTS joinUrl: "+count+"/"+count+" exact UTF-8 outputs passed");
