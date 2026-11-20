import {readFileSync} from "node:fs";
import {resolve} from "node:path";
import {pathToFileURL} from "node:url";
const dist=process.env.GUTTS_DIST;
if(!dist)throw Error("GUTTS_DIST required");
const {resolveSemanticInput}=await import(pathToFileURL(resolve(dist,"semantic-input/index.js")).href);
const rows=readFileSync(new URL("../testdata/upstream-golden/semantic-resolver.tsv",import.meta.url),"utf8")
  .split(/\r?\n/).filter(s=>s&&!s.startsWith("#"));
let cases=0;
for(const line of rows){
 const parts=line.split("\t");
 if(parts.length!==4)throw Error("Malformed deterministic semantic resolver fixture");
 const [requestJSON,expectedStatus,expectedValue,expectedConfirmation]=parts;
 const request=JSON.parse(requestJSON);
 const result=await resolveSemanticInput(request);
 const value=result.value===undefined?"_":JSON.stringify(result.value);
 const confirmation=result.requiresConfirmation===true?"1":"0";
 if(result.status!==expectedStatus||value!==expectedValue||
    confirmation!==expectedConfirmation||!Array.isArray(result.candidates)||
    result.candidates.length)
  throw Error(JSON.stringify({input:request,expectedStatus,expectedValue,
    expectedConfirmation,status:result.status,value,confirmation}));
 cases++;
}
if(cases!==12)throw Error("Expected 12 pinned TS source deterministic semantic cases");
console.log("Pinned GutTS resolveSemanticInput deterministic end-to-end contracts passed: "+cases+
 " (full model-backed async/provenance/cache behavior requires host implementation)");
