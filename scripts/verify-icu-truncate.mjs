// Differential against compiled, immutable GutTS's real Lodash truncate.
// No reimplementation or handwritten expected outputs.
import {execFileSync} from "node:child_process";
import {pathToFileURL} from "node:url";
import {resolve} from "node:path";
const dist=process.env.GUTTS_DIST,runner=process.env.GUTC_ICU_RUNNER;
if(!dist||!runner)throw Error("GUTTS_DIST and GUTC_ICU_RUNNER are required");
const {truncate}=await import(pathToFileURL(resolve(dist,"strings/string-transform.js")).href);
if(typeof truncate!=="function")throw Error("Pinned GutTS does not export truncate");
const samples=[
  "", "A", "ABC", "ABCDE", "ABCDEFGHIJKLMNOPQRSTUVWXYZ",
  "école", "e\u0301cole", "Cafe\u0301s", "😊😊😊",
  "A👨‍👩‍👦🇮🇳é", "👨‍👩‍👧‍👦 family", "🇮🇳🇺🇸🇯🇵",
  "👍🏽👍🏼👍🏻", "a\u0308bc\u0301d", "abcdef 12345",
  "mañana", "naïve café", "λµπ foo", "शिक्षा", "日本語abc"
];
const lengths=[0,1,2,3,4,5,6,8,13,22,40];
const suffixes=["…","--",""];
let count=0;
for(const s of samples)for(const n of lengths)for(const suffix of suffixes){
  const expected=Buffer.from(truncate(s,n,suffix),"utf8");
  const actual=execFileSync(resolve(runner),[String(n),suffix,s],{maxBuffer:131072});
  if(!expected.equals(actual)){
    throw Error(JSON.stringify({source:"GutTS@3e2cbc0990ba29830ce91c5d1a1d552675d5334c",input:s,limit:n,suffix,expected:expected.toString("hex"),actual:actual.toString("hex")}));
  }
  count++;
}
console.log("GutC native ICU grapheme vs compiled immutable pinned GutTS Lodash truncate: "+count+" exact UTF-8 results passed");
