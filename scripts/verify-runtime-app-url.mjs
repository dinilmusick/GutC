import {readFileSync} from "node:fs";
import {resolve} from "node:path";
import {pathToFileURL} from "node:url";
const dir=process.env.GUTTS_DIST;
if(!dir)throw new Error("GUTTS_DIST required");
const {resolveRuntimeAppUrl}=await import(pathToFileURL(resolve(dir,"auth/luthAuthClient.js")).href);
const {resolveServerRuntimeAppUrl}=await import(pathToFileURL(resolve(dir,"server/auth.js")).href);
const lines=readFileSync(new URL("../testdata/upstream-golden/runtime-app-url.tsv",import.meta.url),"utf8")
  .split(/\r?\n/).filter(x=>x&&!x.startsWith("#"));
const oldLocation=globalThis.location;
const oldEnv=process.env.APP_URL;
let checked=0;
try {
  for(const line of lines){
    const parts=line.split("\t");
    if(parts.length!==2)throw new Error("Invalid APP_URL fixture");
    const [input,want]=parts.map(JSON.parse);
    globalThis.location={origin:input??undefined};
    if(input===null)delete process.env.APP_URL;
    else process.env.APP_URL=input;
    const browser=resolveRuntimeAppUrl();
    const server=resolveServerRuntimeAppUrl();
    if(browser!==want||server!==want)
      throw new Error(JSON.stringify({input,want,browser,server}));
    checked++;
  }
}finally{
  globalThis.location=oldLocation;
  if(oldEnv===undefined)delete process.env.APP_URL;
  else process.env.APP_URL=oldEnv;
}
if(checked<30)throw new Error("Missing APP_URL fixtures");
console.log("Pinned GutTS runtime and server APP_URL cases passed: "+checked);
