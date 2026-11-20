import {readFileSync} from "node:fs";
import {resolve} from "node:path";
import {pathToFileURL} from "node:url";
const dist=process.env.GUTTS_DIST;
if(!dist)throw Error("GUTTS_DIST is required");
const {LuthAuthClient}=await import(pathToFileURL(resolve(dist,"auth/luthAuthClient.js")).href);
const rows=readFileSync(new URL("../testdata/upstream-golden/luth-client.tsv",import.meta.url),"utf8")
  .split(/\r?\n/).filter(x=>x&&!x.startsWith("#"));
const expected=rows.map(line=>{
 const [name,method,path,body]=line.split("\t");
 if(!name||!method||!path||body===undefined)throw Error("Malformed fixture");
 return {name,method,path,body};
});
const originalFetch=globalThis.fetch;
const calls=[],tokens={value:""};
const tokenStore={
 get:()=>tokens.value,
 set:t=>{tokens.value=t;},
 clear:()=>{tokens.value="";},
};
let loggedOut=false;
globalThis.fetch=async (url,init={})=>{
 const entry=expected[calls.length];
 if(!entry)throw Error("Unexpected Luth request "+url);
 const path=new URL(url).pathname;
 const method=init.method??"GET";
 if(method!==entry.method||path!==entry.path)
   throw Error(JSON.stringify({entry,method,path}));
 if(init.credentials!=="include"||init.cache!=="no-store")
   throw Error("Missing real GutTS request credential/cache contract");
 if((init.body??"_")!==entry.body)
   throw Error(JSON.stringify({name:entry.name,want:entry.body,got:init.body}));
 calls.push(entry.name);
 if(entry.name==="register")
   return {status:200,ok:true,headers:new Headers(),json:async()=>({
     registered:true,ecosystemId:"eco",applicationId:"app",
     origin:"https://app.example",environment:"production",
     authenticationEnabled:true
   })};
 if(entry.name==="session")
   return {status:200,ok:true,headers:new Headers({"set-auth-token":"header-token"}),
     json:async()=>({authenticated:true,user:{id:"u1",email:"e@example.org"},
       session:{id:"s1"}})};
 if(entry.name==="session-after-signout"){
   if(!loggedOut)throw Error("Session check before signout");
   return {status:401,ok:false,headers:new Headers(),json:async()=>({})};
 }
 if(entry.name==="signout")loggedOut=true;
 const response=entry.name==="signin"?{token:"body-token"}:{success:true};
 return {status:200,ok:true,headers:new Headers(),json:async()=>response};
};
try{
 const client=new LuthAuthClient({
  ecosystemId:"eco",applicationId:"app",origin:"https://app.example",
  luthUrls:["https://luth.example"],autoRegister:false,maxRetries:0,
  tokenStore
 });
 const registered=await client.register();
 if(!registered.registered)throw Error("Expected registration");
 const session=await client.getSession();
 if(!session.authenticated||tokens.value!=="header-token")
   throw Error("Session/token hydration mismatch");
 const signed=await client.signInWithEmail({email:"e@example.org",password:"pa\\ss\"123"});
 if(signed.token!=="body-token"||tokens.value!=="body-token")
   throw Error("Email signin token mismatch");
 await client.signUpWithEmail({email:"e@example.org",password:"password2",name:"A Woman"});
 await client.signInWithGoogle();
 await client.signOut();
 if(tokens.value)throw Error("Signout did not clear token");
 if(JSON.stringify(calls)!==JSON.stringify(expected.map(v=>v.name)))
   throw Error("Unexpected request sequence");
}finally{globalThis.fetch=originalFetch;}
console.log("Pinned GutTS LuthAuthClient HTTP session/register/auth/logout contracts passed: "+calls.length);
