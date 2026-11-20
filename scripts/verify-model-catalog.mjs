import {readFileSync} from "node:fs";
import {resolve} from "node:path";
import {pathToFileURL} from "node:url";
const dir=process.env.GUTTS_DIST;
if(!dir)throw new Error("GUTTS_DIST required");
const {guttsModelDefinitions,guttsModelById}=await import(pathToFileURL(resolve(dir,"models/model-registry.js")).href);
const expected=JSON.parse(readFileSync(new URL("../testdata/upstream-golden/model-catalog.json",import.meta.url),"utf8"));
if(expected.length!==8)throw new Error("Expected 8 pinned model definitions");
if(JSON.stringify(guttsModelDefinitions)!==JSON.stringify(expected))
  throw new Error("Model metadata snapshot differs from pinned real GutTS source");
for(const model of expected) {
  if(JSON.stringify(guttsModelById[model.id])!==JSON.stringify(model))
    throw new Error("Model lookup differs for "+model.id);
}
console.log("Pinned GutTS model registry metadata fixtures passed: "+expected.length);
