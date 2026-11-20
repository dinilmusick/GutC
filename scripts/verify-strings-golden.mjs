import {readFileSync} from "node:fs";
import {resolve} from "node:path";
import {pathToFileURL} from "node:url";
const dir=process.env.GUTTS_DIST;
if(!dir)throw new Error("GUTTS_DIST required");
const strings=await import(pathToFileURL(resolve(dir,"strings/string-concepts.js")).href);
const lines=readFileSync(new URL("../testdata/upstream-golden/strings.tsv",import.meta.url),"utf8")
  .split(/\r?\n/).filter(line=>line&&!line.startsWith("#"));
let checked=0;
for(const row of lines) {
  const first=row.indexOf("\t"),last=row.lastIndexOf("\t");
  if(first<0||first===last)throw new Error("Bad strings golden fixture");
  const op=row.slice(0,first),input=row.slice(first+1,last),expected=row.slice(last+1);
  let result;
  if(op==="email")result=strings.isEmail(input)?"1":"0";
  else if(op==="capitalize")result=strings.capitalize(input);
  else if(op==="camel")result=strings.camelCase(input);
  else if(op==="kebab")result=strings.kebabCase(input);
  else if(op==="snake")result=strings.snakeCase(input);
  else throw new Error("Unknown strings fixture operation "+op);
  if(result!==expected)throw new Error(JSON.stringify({op,input,expected,result}));
  checked++;
}
if(checked<30)throw new Error("Incomplete strings golden corpus");
console.log("Pinned GutTS strings golden cases passed: "+checked);
