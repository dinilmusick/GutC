import {readFileSync} from "node:fs";
import {resolve} from "node:path";
import {pathToFileURL} from "node:url";
const dist=process.env.GUTTS_DIST;
if(!dist)throw new Error("GUTTS_DIST required");
const root=resolve(dist,"../src/models");
const sourceFiles={
 Needle3:"needle3/index.ts",SmolLM2:"smollm2/index.ts",
 Whistle:"whistle/index.ts",MiniLM:"minilm/index.ts",
 EttinReranker17M:"ettin/index.ts",EttinReranker32M:"ettin/index.ts",
 EttinReranker68M:"ettin/index.ts",EttinReranker150M:"ettin/index.ts",
};
const modelRegistry=await import(pathToFileURL(resolve(dist,"models/model-registry.js")).href);
const known=new Set(modelRegistry.guttsModelDefinitions.map(d=>d.id));
const lines=readFileSync(new URL("../testdata/upstream-golden/model-operations.tsv",import.meta.url),"utf8")
 .split(/\r?\n/).filter(x=>x&&!x.startsWith("#"));
let cases=0;
for(const line of lines){
 const parts=line.split("\t");
 if(parts.length!==5)throw new Error("Malformed model operation fixture");
 const [name,id,method,jsonInput,jsonOutput]=parts;
 const source=sourceFiles[name];
 if(!source)throw new Error("Unknown pinned model class "+name);
 const text=readFileSync(resolve(root,source),"utf8");
 if(!text.includes("export class "+name+" ")||
    !text.includes("\n  "+method+"("))
  throw new Error("Pinned GutTS class/method missing "+name+"."+method);
 if(!known.has(id))throw new Error("Pinned model metadata ID missing "+id);
 const input=JSON.parse(jsonInput),output=JSON.parse(jsonOutput);
 if(!input||typeof input!=="object"||Array.isArray(input))
  throw new Error("Model test input must be structured object");
 if(method==="complete"||method==="generate"){
  if(typeof input.prompt!=="string")throw new Error("Missing prompt");
 }
 if(method==="rerank"){
  if(typeof input.query!=="string"||!Array.isArray(input.candidates)||
     !Array.isArray(output)||output.length!==input.candidates.length)
   throw new Error("Rerank input/output fixture invalid");
 }
 if(method==="similarity"&&(!Number.isFinite(output)||typeof input.left!=="string"||
                             typeof input.right!=="string"))
  throw new Error("Similarity fixture invalid");
 if(method==="transcribe"&&(!Array.isArray(input.audio)||typeof output!=="string"))
  throw new Error("Transcription fixture invalid");
 if(method==="embed"&&(!Array.isArray(output)||typeof input.input!=="string"))
  throw new Error("Embedding fixture invalid");
 cases++;
}
if(cases!==9)throw new Error("Expected nine pinned class-operation cases");
console.log("Pinned GutTS model class/method/metadata contracts passed: "+cases+
 " (test host values only, no model inference)");
