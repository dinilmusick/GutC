import {readFileSync} from "node:fs";
const lines=readFileSync(new URL("../testdata/upstream-golden/semantic-rank.tsv",import.meta.url),"utf8")
  .split(/\r?\n/).filter(x=>x&&!x.startsWith("#"));
let checked=0;
for(const row of lines){
  const values=row.split("\t");
  if(values.length!==4)throw new Error("Malformed semantic-rank fixture");
  const [candidates,similarities,relevance,want]=values.map(JSON.parse);
  // Exact comparator body from pinned GutTS semantic-input/index.ts.
  // Model generation is intentionally NOT substituted with stub scores.
  const sorted=candidates
    .map((value,index)=>({value,index,similarity:similarities[index],relevance:relevance[index]}))
    .sort((a,b)=>{
      const relDiff=(b.relevance??-Infinity)-(a.relevance??-Infinity);
      if(relDiff!==0)return relDiff;
      const simDiff=(b.similarity??-Infinity)-(a.similarity??-Infinity);
      if(simDiff!==0)return simDiff;
      return a.value.localeCompare(b.value);
    }).map(x=>x.index);
  if(JSON.stringify(sorted)!==JSON.stringify(want))
    throw new Error("Semantic comparator mismatch "+row);
  checked++;
}
if(checked<20)throw new Error("Insufficient ranking comparator fixtures");
console.log("Pinned semantic ranking comparator fixtures passed: "+checked+" (source comparator, not model inference)");
