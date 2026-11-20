import {readFileSync} from "node:fs";
import {resolve} from "node:path";
import {pathToFileURL} from "node:url";
const dir=process.env.GUTTS_DIST;
if(!dir)throw new Error("GUTTS_DIST required");
const contract=await import(pathToFileURL(resolve(dir,"auth/oauthClientContract.js")).href);
const lines=readFileSync(new URL("../testdata/upstream-golden/contract-primitives.tsv",import.meta.url),"utf8")
  .split(/\r?\n/).filter(x=>x&&!x.startsWith("#"));
let checked=0;
for(const line of lines) {
  const values=line.split("\t");
  if(values.length!==4)throw new Error("Malformed contract primitive fixture");
  const [op,raw,keys,expect]=values;
  const value=JSON.parse(raw),allowed=JSON.parse(keys),want=JSON.parse(expect);
  let actual;
  if(op==="record")actual=!!contract.contractRecord(value);
  else if(op==="id")actual=contract.contractId(value);
  else if(op==="appId")actual=contract.contractAppId(value);
  else if(op==="keys")actual=contract.contractExactKeys(value,allowed);
  else throw new Error("Unknown op "+op);
  if(actual!==want)throw new Error(JSON.stringify({op,raw,keys,want,actual}));
  checked++;
}
if(checked<60)throw new Error("Not enough pinned contract primitive cases");
if(contract.ULTRST_APP_CONTRACT_VERSION!==1)throw new Error("Contract version mismatch");
console.log("Pinned GutTS primitive contract helpers passed: "+checked);
