import {readFileSync} from "node:fs";
import {resolve} from "node:path";
import {pathToFileURL} from "node:url";
const dir=process.env.GUTTS_DIST;
if(!dir)throw new Error("GUTTS_DIST required");
const {ApplicationAuthClient}=await import(pathToFileURL(resolve(dir,"auth/applicationAuthClient.js")).href);
const rows=readFileSync(new URL("../testdata/upstream-golden/application-auth.tsv",import.meta.url),"utf8")
  .split(/\r?\n/).filter(x=>x&&!x.startsWith("#"));
const oldWindow=globalThis.window;
let checked=0;
try{
for(const line of rows){
 const parts=line.split("\t");
 if(parts.length!==5)throw new Error("Malformed ApplicationAuthClient case");
 const [base,returnTo,current,expectedUrl,expectedError]=parts.map(JSON.parse);
 globalThis.window=current===null?undefined:{location:{pathname:current,search:"",hash:""}};
 let navigated="";
 let error="";
 try{
   const client=new ApplicationAuthClient({
     basePath:base,
     navigate:path=>{navigated=path;},
     fetcher:async()=>{throw new Error("unexpected network");},
   });
   client.startSignIn(returnTo===null?undefined:returnTo);
 }catch(e){error=e?.code??"UNEXPECTED_EXCEPTION";}
 if(navigated!==expectedUrl||error!==expectedError)
   throw new Error(JSON.stringify({base,returnTo,current,expectedUrl,expectedError,navigated,error}));
 checked++;
}
const valid={
 authenticated:true,
 user:{id:"user-1",email:"a@example.org"},
 session:{id:"session-1"},
};
let afterLogout=false,requests=0;
const client=new ApplicationAuthClient({
 basePath:"/api/auth/oidc",
 navigate:()=>{throw new Error("unexpected navigation");},
 fetcher:async (url,init)=>{
   requests++;
   if(init?.credentials!=="same-origin"||init?.cache!=="no-store")
     throw new Error("credentials or cache settings diverged");
   if(url==="/api/auth/oidc/session"&&init?.method==="GET")
     return afterLogout?{status:401,ok:false}:{status:200,ok:true,json:async()=>valid};
   if(url==="/api/auth/oidc/logout"&&init?.method==="POST"&&init?.body==="{}"){
     afterLogout=true;return {status:204,ok:true};
   }
   throw new Error("unexpected auth fetch "+url);
 },
});
const result=await client.getSession();
if(!result.authenticated||result.user?.id!=="user-1")throw new Error("invalid session");
await client.signOut();
if(requests!==3)throw new Error("Expected session, logout, verify sequence");
const invalid=new ApplicationAuthClient({
 fetcher:async()=>({status:200,ok:true,json:async()=>({authenticated:true,user:{id:""},session:{id:"s"}})}),
});
try {await invalid.getSession();throw new Error("Missing AUTH_SESSION_INVALID");}
catch(e){if(e?.code!=="AUTH_SESSION_INVALID")throw e;}
}finally{globalThis.window=oldWindow;}
if(checked<35)throw new Error("Not enough auth navigation cases");
console.log("Pinned GutTS application auth navigation fixtures passed: "+checked);
console.log("Pinned application auth session/logout host transport sequence verified");
