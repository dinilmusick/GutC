import {readFileSync} from "node:fs";
import {resolve} from "node:path";
import {pathToFileURL} from "node:url";
const dist=process.env.GUTTS_DIST;
if(!dist)throw new Error("GUTTS_DIST required");
const {ModelResidency}=await import(pathToFileURL(resolve(dist,"models/model-residency.js")).href);
const {ModelLibrary}=await import(pathToFileURL(resolve(dist,"models/model-library.js")).href);
const {guttsModelDefinitions}=await import(pathToFileURL(resolve(dist,"models/model-registry.js")).href);
const lines=readFileSync(new URL("../testdata/upstream-golden/model-residency.tsv",import.meta.url),"utf8")
 .split(/\r?\n/).filter(x=>x&&!x.startsWith("#"));
let tested=0;
for(const line of lines){
 const [scenario,expected]=line.split("\t");
 const events=[],residency=new ModelResidency(),model={id:"test-handle"};
 const load=async()=>{events.push("load");if(scenario==="load-failure")throw Error("load failed");return model;};
 const unload=async()=>{events.push("unload");};
 const op=async m=>{if(m!==model)throw Error("wrong model handle");events.push("operation");return 42;};
 try{
  if(scenario==="sequential"){
   if(await residency.run(load,unload,op)!==42)throw Error("bad return");
   if(await residency.run(load,unload,op)!==42)throw Error("bad return");
  }else if(scenario==="nested"){
   await residency.run(load,unload,async outer=>{
    if(outer!==model||residency.activeUsers!==1)throw Error("bad outer lease");
    events.push("outer");
    await residency.run(load,unload,async inner=>{
     if(inner!==model||residency.activeUsers!==2)throw Error("bad nested lease");
     events.push("operation");
    });
   });
  }else if(scenario==="operation-failure"){
   try{await residency.run(load,unload,async m=>{await op(m);throw Error("operation failed");});}
   catch(e){if(e.message!=="operation failed")throw e;}
  }else if(scenario==="load-failure"){
   try{await residency.run(load,unload,op);}catch(e){if(e.message!=="load failed")throw e;}
  }else if(scenario==="library"){
   const runtime={load:async()=>load(),unload:async()=>unload()};
   class TestLibrary extends ModelLibrary{
    constructor(){super(guttsModelDefinitions[0],runtime);}
    execute(){return this.withModel(op);}
   }
   const library=new TestLibrary();
   if(await library.execute()!==42)throw Error("wrong library result");
  }else throw Error("Unknown scenario "+scenario);
 }finally {
  if(residency.activeUsers!==0||residency.isLoaded)throw Error("Leaked pinned ModelResidency");
 }
 const actual=events.join(",");
 if(actual!==expected)throw Error(JSON.stringify({scenario,expected,actual}));
 tested++;
}
if(tested!==5)throw Error("Incomplete model residency fixture");
console.log("Pinned GutTS ModelResidency and ModelLibrary model-host lifecycle fixtures passed: "+tested);
