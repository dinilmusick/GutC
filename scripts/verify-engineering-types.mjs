import {readFileSync} from "node:fs";
import {resolve} from "node:path";
const dist=process.env.GUTTS_DIST;
if(!dist)throw new Error("GUTTS_DIST required");
const code=readFileSync(resolve(dist,"../src/engineering/engineering-types.ts"),"utf8");
const corpus=readFileSync(new URL("../testdata/upstream-golden/engineering-kinds.tsv",import.meta.url),"utf8")
 .split(/\r?\n/).filter(x=>x&&!x.startsWith("#"));
const quoted=s=>[...s.matchAll(/"([^"]+)"/g)].map(x=>x[1]);
const match=(re)=>{
 const m=code.match(re);
 if(!m)throw new Error("Pinned GutTS type union not present: "+re);
 return quoted(m[1]);
};
const kinds={
 provider:match(/export type ProviderKind\s*=\s*([\s\S]*?);/),
 capability:match(/export type EngineeringCapability\s*=\s*([\s\S]*?);/),
 network:match(/network\?:\s*([^;]+);/),
 "execution-status":match(/export interface ExecutionResult[\s\S]*?status:\s*([^;]+);/),
 "check-status":match(/export interface VerificationCheck[\s\S]*?status:\s*([^;]+);/),
 "report-status":match(/export interface VerificationReport[\s\S]*?status:\s*([^;]+);/),
};
const expected=Object.entries(kinds).flatMap(([group,values])=>values.map((value,i)=>group+"\t"+value+"\t"+i));
if(JSON.stringify(corpus)!==JSON.stringify(expected))
 throw new Error("Native engineering enum fixture disagrees with exact pinned GutTS source");
const shapes={
 RepositoryRef:["id","provider","owner","name","defaultBranch","remote"],
 WorkspaceRef:["id","root","repositories","snapshotId"],
 ExecutionRequest:["workspaceId","command","cwd","environment","timeoutMs","network","resources"],
 ExecutionResult:["executionId","status","exitCode","stdout","stderr","artifactIds"],
 VerificationCheck:["id","name","status","durationMs","executionId","details"],
 VerificationReport:["id","status","checks","artifactIds"],
 ArtifactRef:["id","digest","type","uri","sourceExecutionId"],
 CapabilityGrant:["capability","resource","expiresAt"],
 EngineeringOperationContext:["taskId","traceId","workspaceId","grants"],
 ProvenanceRecord:["operationId","actorId","taskId","traceId","timestamp","source","commit","artifacts"],
};
for(const [name,fields] of Object.entries(shapes)){
 const marker="export interface "+name+" {";
 const index=code.indexOf(marker);
 if(index<0)throw new Error("Missing source interface "+name);
 const next=code.indexOf("\nexport ",index+marker.length);
 const section=code.slice(index,next<0?undefined:next);
 for(const field of fields)
  if(!new RegExp("\\b"+field+"\\s*\\??\\s*:").test(section))
    throw new Error("Pinned GutTS "+name+" lacks expected property "+field);
}
if(!code.includes("export type Identifier = string;"))
 throw new Error("Identifier alias changed");
console.log("Pinned GutTS engineering types checked: "+Object.keys(shapes).length+" interfaces, "+corpus.length+" enum values");
