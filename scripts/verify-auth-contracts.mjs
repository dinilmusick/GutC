import {readFileSync} from "node:fs";
import {resolve} from "node:path";
import {pathToFileURL} from "node:url";
const dir=process.env.GUTTS_DIST;
if(!dir)throw new Error("GUTTS_DIST environment variable required");
const auth=await import(pathToFileURL(resolve(dir,"auth/index.js")).href);
const input=readFileSync(new URL("../testdata/upstream-golden/auth.tsv",import.meta.url),"utf8");
const rows=input.split(/\r?\n/).filter(x=>x && !x.startsWith("#"));
let verified=0;
for(const row of rows) {
  const first=row.indexOf("\t"), last=row.lastIndexOf("\t");
  if(first<0||first===last)throw new Error("Malformed auth contract fixture");
  const op=row.slice(0,first), json=JSON.parse(row.slice(first+1,last));
  const expected=row.slice(last+1);
  let result;
  if(op==="capability")result=auth.validateCapabilityDescriptor(json);
  else if(op==="invocation")result=auth.validateScopedInvocation(json);
  else if(op.startsWith("budget:"))result=auth.checkInvocationBudget(json,Number(op.slice(7)));
  else if(op.startsWith("match:"))result=auth.validateInvocationAgainstDescriptor(
    json.invocation,json.descriptor,Number(op.slice(6)));
  else throw new Error("Unrecognized auth fixture operation: "+op);
  const received=result.ok?"OK":result.reason;
  if(received!==expected)throw new Error(JSON.stringify({op,expected,received,json}));
  verified++;
}
if(verified<25)throw new Error("Expected at least 25 auth fixtures; got "+verified);
console.log("Pinned GutTS auth contract fixtures passed: "+verified);
