import {readFileSync} from "node:fs";
import {resolve} from "node:path";
import {pathToFileURL} from "node:url";
const dir=process.env.GUTTS_DIST;
if(!dir)throw new Error("GUTTS_DIST required");
const auth=await import(pathToFileURL(resolve(dir,"auth/index.js")).href);
const input=readFileSync(new URL("../testdata/upstream-golden/oauth.tsv",import.meta.url),"utf8");
const lines=input.split(/\r?\n/).filter(x=>x&&!x.startsWith("#"));
let cases=0;
for(const line of lines) {
  const first=line.indexOf("\t"),last=line.lastIndexOf("\t");
  if(first<0||first===last)throw new Error("Bad OAuth golden row");
  const operation=line.slice(0,first),payload=JSON.parse(line.slice(first+1,last));
  const expected=line.slice(last+1);
  let result;
  if(operation==="registration")result=auth.validateOAuthClientRegistration(payload);
  else if(operation.startsWith("redirect:"))
    result=auth.validateOAuthRedirectUri(payload,operation.slice(9));
  else throw new Error("Unknown OAuth golden operation "+operation);
  const actual=result.ok?"OK":result.reason;
  if(actual!==expected)throw new Error(JSON.stringify({operation,payload,expected,actual}));
  cases++;
}
if(cases<25)throw new Error("Insufficient OAuth golden cases");
console.log("Pinned GutTS OAuth golden cases passed: "+cases);
