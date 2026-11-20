import { readFileSync } from "node:fs";
import { join, resolve } from "node:path";
import { pathToFileURL } from "node:url";
const root=process.env.GUTTS_DIST;
if(!root) throw new Error("GUTTS_DIST must point to compiled pinned GutTS/dist");
const load=async p=>import(pathToFileURL(resolve(root,p)).href);
const [identity,strings,urls,paths,numbers,arrays]=await Promise.all([
  load("auth/identityIds.js"),load("strings/string-concepts.js"),
  load("urls/url-concepts.js"),load("urls/path-safety.js"),
  load("numbers/number-concepts.js"),load("arrays/array-concepts.js")
]);
const rows=readFileSync(join(import.meta.dirname,"..","testdata","upstream-golden","contracts.tsv"),"utf8")
  .split(/\r?\n/).filter(line=>line.length&&line[0]!=="#");
let passed=0;
for(const row of rows){
  const fields=row.split("\t");
  if(fields.length!==4) throw new Error("Malformed fixture: "+row);
  const [op,input,arg,expected]=fields;
  let got;
  switch(op){
    case "identity": got=String(Number(identity.parseIdentityId("application",input)!==null));break;
    case "blank": got=String(Number(strings.isBlank(input)));break;
    case "starts": got=String(Number(strings.startsWithAny(input,[arg])));break;
    case "ends": got=String(Number(strings.endsWithAny(input,[arg])));break;
    case "join": got=urls.joinUrl(input,arg);break;
    case "safePath": got=paths.encodeSafePathSegments(input);break;
    case "words": got=strings.truncateWords(input,Number(arg));break;
    case "inRange": {const [lo,hi]=arg.split(":").map(Number);got=String(Number(numbers.inRange(Number(input),lo,hi)));break;}
    case "clamp": {const [lo,hi]=arg.split(":").map(Number);got=String(numbers.clamp(Number(input),lo,hi));break;}
    case "sum": got=String(numbers.sum(input.split(",").map(Number)));break;
    case "unique": got=arrays.unique(input.split(",").map(Number)).join(",");break;
    default: throw new Error("Unknown fixture operation: "+op);
  }
  if(got!==expected) throw new Error("GutTS golden drift: "+JSON.stringify({op,input,arg,got,expected}));
  passed++;
}
if(passed<20) throw new Error("Golden corpus unexpectedly incomplete: "+passed);
console.log("Pinned GutTS golden fixture checks passed:",passed);
