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
let count=0,exact=0;const knownDifferences=[];
for(const s of samples)for(const n of lengths)for(const suffix of suffixes){
  const expected=Buffer.from(truncate(s,n,suffix),"utf8");
  const actual=execFileSync(resolve(runner),[String(n),suffix,s],{maxBuffer:131072});
  if(!expected.equals(actual)){
    const evidence={input:s,limit:n,suffix,expected:expected.toString("hex"),actual:actual.toString("hex")};
    // Lodash's pinned Unicode-symbol regex does not implement ICU UAX #29:
    // the Devanagari vowel mark ि is counted separately by Lodash.
    // This is a known explicit source-compatibility gap, not a passing match.
    if(s!=="शिक्षा")throw Error(JSON.stringify({unexpected_unicode_divergence:evidence}));
    knownDifferences.push(evidence);
  }else exact++;
  count++;
}
if(count!==660||!knownDifferences.some(d=>d.limit===1&&d.suffix===""&&
    d.expected==="e0a4b6"&&d.actual==="e0a4b6e0a4bf"))
  throw Error("Missing required known Lodash-vs-ICU Devanagari witness");
console.log("GutC pinned source differential: "+exact+"/"+count+" byte-for-byte matches, "+
 knownDifferences.length+" KNOWN SEMANTIC DIFFERENCES for Devanagari Unicode; full Lodash parity is NOT certified.");
for(const diff of knownDifferences)console.log("KNOWN_DIVERGENCE "+JSON.stringify(diff));
