// Strict native C11 ICU-regex vs actual compiled pinned GutTS Lodash Unicode
// truncation. No handwritten expected text; all outcomes are source-executed.
import {execFileSync} from "node:child_process";
import {pathToFileURL} from "node:url";
import {resolve} from "node:path";
const compiled=process.env.GUTTS_DIST;
const runner=process.env.GUTC_LODASH_RUNNER;
if(!compiled||!runner)throw Error("GUTTS_DIST/GUTC_LODASH_RUNNER missing");
const {truncate}=await import(pathToFileURL(resolve(compiled,"strings/string-transform.js")).href);
if(typeof truncate!=="function")throw Error("The pinned compiled source no longer exports strings.truncate");
const samples=[
 "", "A", "ABC", "ABCDE", "ABCDEFGHIJKLMNOPQRSTUVWXYZ",
 "école", "e\u0301cole", "Cafe\u0301s", "😊😊😊", "A👨‍👩‍👦🇮🇳é",
 "👨‍👩‍👧‍👦 family", "🇮🇳🇺🇸🇯🇵", "👍🏽👍🏼👍🏻", "a\u0308bc\u0301d",
 "abcdef 12345", "mañana", "naïve café", "λµπ foo", "शिक्षा",
 "日本語abc", "क्‍ष", "क्‍षत्र", "प्रयोग", "संस्कृत",
 "മലയാളം", "ചന്ദ്രൻ", "தமிழ்", "తెలుగు", "ಕನ್ನಡ",
 "বাংলা", "ગુજરાતી", "ਪੰਜਾਬੀ", "العَرَبِيَّة", "שָׁלוֹם",
 "ภาษาไทย", "ລາວ", "རྒྱ་གར", "조선어",
 "🤝🏽", "👩🏽‍💻", "🏳️‍🌈", "1️⃣2️⃣3️⃣",
 "👩‍❤️‍💋‍👨", "🇮🇳🏳️‍🌈e\u0301क्‍ष", "a👩🏽‍💻Z",
 "👨‍👩‍👧‍👦🇯🇵👩🏽‍💻", "\u200dabc",
 "a\u200d\u200db", "\u0300\u0301x", "\ufe0f\u200d\u0301"
];
const lengths=[0,1,2,3,4,5,6,8,13,22,40,80];
const suffixes=["…","--",""];
let checked=0;
for(const value of samples)for(const max of lengths)for(const omission of suffixes){
  const source=Buffer.from(truncate(value,max,omission),"utf8");
  const native=execFileSync(resolve(runner),[String(max),omission,value],{
    maxBuffer:131072,timeout:10000
  });
  if(!native.equals(source))
    throw Error(JSON.stringify({
      problem:"Pinned Lodash native Unicode incompatibility",
      upstream:"dinilmusick/GutTS@3e2cbc0990ba29830ce91c5d1a1d552675d5334c",
      value,max,omission,source_hex:source.toString("hex"),native_hex:native.toString("hex")
    }));
  checked++;
}
if(checked!==samples.length*lengths.length*suffixes.length)
  throw Error("Differential corpus length unexpectedly changed");
console.log("GutC optional native Lodash compatibility: "+checked+"/"+checked+
  " exact UTF-8 results from actual compiled immutable pinned GutTS Lodash");
