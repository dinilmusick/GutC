import {readFileSync} from "node:fs";
import {resolve} from "node:path";
const dist=process.env.GUTTS_DIST;
if(!dist)throw new Error("GUTTS_DIST required");
const semantic=readFileSync(resolve(dist,"../src/semantic-input/index.ts"),"utf8");
const api=readFileSync(resolve(dist,"../src/api/api-types.ts"),"utf8");
const json=readFileSync(resolve(dist,"../src/json/json-value.ts"),"utf8");
const lines=readFileSync(new URL("../testdata/upstream-golden/portable-type-kinds.tsv",import.meta.url),"utf8")
 .split(/\r?\n/).filter(x=>x&&!x.startsWith("#"));
const strings=s=>[...s.matchAll(/"([^"]+)"/g)].map(x=>x[1]);
function sourceSet(regex) {
 const match=semantic.match(regex);
 if(!match)throw Error("Pinned semantic export changed: "+regex);
 return strings(match[1]);
}
const eventStart=semantic.indexOf("export type SemanticInputEvent =");
const eventEnd=semantic.indexOf("export type SemanticInputModelPolicy =",eventStart);
if(eventStart<0||eventEnd<0)throw Error("Semantic event type missing");
const events=[...semantic.slice(eventStart,eventEnd).matchAll(/\btype:\s*"([^"]+)"/g)].map(x=>x[1]);
const groups={
 error:sourceSet(/export type SemanticInputErrorCode\s*=\s*([\s\S]*?);/),
 risk:sourceSet(/export type SemanticInputRisk\s*=\s*([^;]+);/),
 phase:sourceSet(/export type SemanticInputProgress\s*=\s*\{([\s\S]*?)\};/).slice(0,5),
 event:events,
 status:sourceSet(/export type SemanticInputResolution<T\s*=\s*unknown>\s*=\s*\{\s*status:\s*([^;]+);/),
 policy:sourceSet(/export type SemanticInputModelPolicy\s*=\s*([^;]+);/),
};
const expected=Object.entries(groups).flatMap(([group,values])=>values.map((value,i)=>group+"\t"+value+"\t"+i));
if(JSON.stringify(expected)!==JSON.stringify(lines))
 throw Error("Native semantic type literals differ from exact pinned GutTS source");
const requiredApi={
 JsonSchema:["type","description","properties","items","required","additionalProperties","enum"],
 ApiParameter:["name","description","required","schema"],
 ApiSemantics:["deterministic","pure","sideEffects"],
 ApiExample:["input","output"],
 ApiDefinition:["id","name","category","description","parameters","output","semantics","errors","examples","inputSchema"],
};
const requiredSemantic={
 SemanticInputSchema:["type","properties","required","enum","items","description"],
 SemanticInputContext:["application","workspace","page","section","form","field","label","description","semanticType","vocabulary","examples","aliases","constraints","locale","relatedFields"],
 SemanticInputProgress:["phase","detail"],
 SemanticInputPolicy:["risk","requireConfirmation","modelPolicy"],
 SemanticInputProvenanceStore:["append"],
 SemanticInputProvenanceRecord:["provenance","status","timestamp"],
 SemanticInputProvenance:["requestFingerprint","resolver","modelPolicy","models","deterministic"],
 SemanticInputCandidate:["value","similarity","relevance"],
 SemanticInputEvidence:["needleOutput","candidates","selectedCandidate","similarityThreshold","relevanceMarginThreshold"],
 SemanticInputClarification:["question","candidates"],
 SemanticInputResolution:["status","value","rawInput","candidates","modelOutput","evidence","clarification","error","requiresConfirmation","provenance"],
 SemanticInputRequest:["input","schema","context","candidates","preserveRawInput","minCandidateSimilarity","minCandidateMargin","policy","cache","signal","onEvent","onProgress","provenanceStore"],
};
function checkTypes(contents,contracts){
 for(const [name,props] of Object.entries(contracts)){
  const startMatch=new RegExp("export type\\s+"+name+"(?:<[^;]+?>)?\\s*=\\s*\\{").exec(contents);
  if(!startMatch)throw Error("Pinned type missing "+name);
  const start=startMatch.index;
  const after=contents.indexOf("\nexport ",start+startMatch[0].length);
  const body=contents.slice(start,after<0?undefined:after);
  for(const prop of props)
   if(!new RegExp("\\b"+prop+"\\s*\\??\\s*[:(]").test(body))
    throw Error("Pinned type "+name+" lacks "+prop);
 }
}
checkTypes(api,requiredApi);
checkTypes(semantic,requiredSemantic);
if(events.length!==7||!semantic.includes('export type SemanticInputEvent ='))
 throw Error('Missing pinned discriminated semantic event type');
for(const name of ["SemanticInputErrorCode","SemanticInputRisk","SemanticInputModelPolicy","SemanticInputCacheKey"])
 if(!new RegExp("export type\\s+"+name+"\\s*=").test(semantic))
  throw Error("Pinned semantic alias missing "+name);
for(const name of ["JsonPrimitive","JsonValue","JsonObject"])
 if(!new RegExp("export type\\s+"+name+"\\s*=").test(json))
  throw Error("Pinned JSON alias missing "+name);
console.log("Pinned GutTS portable types verified: "+Object.keys(requiredApi).length+" API schemas, "+Object.keys(requiredSemantic).length+" semantic structures, 3 JSON types, "+lines.length+" literal variants");
