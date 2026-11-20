import {readFileSync} from "node:fs";
import {resolve} from "node:path";
import {pathToFileURL} from "node:url";
const dir=process.env.GUTTS_DIST;
if(!dir)throw new Error("GUTTS_DIST required");
const {hydrateLuthTokenFromUrl}=await import(pathToFileURL(resolve(dir,"auth/luthAuthClient.js")).href);
const rows=readFileSync(new URL("../testdata/upstream-golden/luth-callback-token.tsv",import.meta.url),"utf8")
  .split(/\r?\n/).filter(x=>x&&!x.startsWith("#"));
const oldLocation=globalThis.location;
const oldHistory=globalThis.history;
const oldDocument=globalThis.document;
let checked=0;
try {
  for(const line of rows){
    const fields=line.split("\t");
    if(fields.length!==5)throw new Error("Malformed callback token fixture");
    const [pathname,search,hash,expectedToken,expectedUrl]=fields.map(JSON.parse);
    let stored=null,rewritten=null;
    globalThis.location={
      origin:"https://example.org",
      pathname,search,hash,
      href:"https://example.org"+pathname+search+hash,
    };
    globalThis.history={replaceState(_state,_title,url){rewritten=url;}};
    globalThis.document={title:"GutC source fixture"};
    const tokenStore={get:()=>stored??"",set(token){stored=token;},clear(){stored=null;}};
    const hydrated=hydrateLuthTokenFromUrl(tokenStore);
    const want=expectedToken!==null;
    if(hydrated!==want||stored!==expectedToken||rewritten!==expectedUrl)
      throw new Error(JSON.stringify({pathname,search,hash,want,hydrated,stored,expectedToken,rewritten,expectedUrl}));
    checked++;
  }
}finally {
  globalThis.location=oldLocation;
  globalThis.history=oldHistory;
  globalThis.document=oldDocument;
}
if(checked<30)throw new Error("Missing pinned callback cases");
console.log("Pinned GutTS Luth callback token cases passed: "+checked);
