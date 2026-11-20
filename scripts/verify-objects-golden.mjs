import {readFileSync} from "node:fs";
import {resolve} from "node:path";
import {pathToFileURL} from "node:url";
const dir=process.env.GUTTS_DIST;
if(!dir)throw new Error("GUTTS_DIST required");
const object=await import(pathToFileURL(resolve(dir,"objects/object-concepts.js")).href);
const rows=readFileSync(new URL("../testdata/upstream-golden/objects.tsv",import.meta.url),"utf8")
  .split(/\r?\n/).filter(x=>x&&!x.startsWith("#"));
let passed=0;
for(const row of rows) {
  const parts=row.split("\t");
  if(parts.length!==4)throw new Error("Malformed object golden row");
  const [op,src,paths,expected]=parts;
  const input=JSON.parse(src),keys=JSON.parse(paths);
  let value;
  if(op==="has")value=object.hasOwn(input,keys[0]);
  else if(op==="pick")value=object.pick(input,keys);
  else if(op==="omit")value=object.omit(input,keys);
  else if(op==="plain")value=object.isPlainObject(input);
  else throw new Error("Unknown object operation "+op);
  const actual=JSON.stringify(value);
  if(actual!==expected)throw new Error(JSON.stringify({op,src,paths,expected,actual}));
  passed++;
}
if(passed<30)throw new Error("Object golden fixture corpus unexpectedly incomplete");
console.log("Pinned GutTS object-path golden cases passed: "+passed);
