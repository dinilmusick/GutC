import {readFileSync} from "node:fs";
import {resolve} from "node:path";
const dist=process.env.GUTTS_DIST;
if(!dist)throw new Error("GUTTS_DIST required");
const root=resolve(dist,"../src/auth");
const files={
 identity:readFileSync(resolve(root,"identityIds.ts"),"utf8"),
 luth:readFileSync(resolve(root,"luthAuthTypes.ts"),"utf8"),
 oauth:readFileSync(resolve(root,"oauthClientContract.ts"),"utf8"),
 capability:readFileSync(resolve(root,"capabilityContract.ts"),"utf8"),
};
const quoted=s=>[...s.matchAll(/['"]([^'"]+)['"]/g)].map(x=>x[1]);
const match=(file,re)=>{const m=files[file].match(re);if(!m)throw new Error("Missing pinned source union "+re);return quoted(m[1]);};
const groups={
 identity:match("identity",/IDENTITY_KINDS\s*=\s*\[([\s\S]*?)\]\s*as const/),
 environment:match("luth",/DeploymentEnvironment\s*=\s*([^;]+);/),
 "oauth-platform":match("oauth",/OAuthPlatform\s*=\s*([^;]+);/),
 parameter:match("capability",/CapabilityParameterKind\s*=\s*([^;]+);/),
 "capability-kind":[...match("capability",/kind:\s*('backend'\s*\|\s*'frontend');/),...match("capability",/kind:\s*('workflow');/)],
 permission:match("capability",/sessionPermission\?:\s*([^;]+);/),
};
const expected=Object.entries(groups).flatMap(([kind,values])=>values.map((value,i)=>kind+"\t"+value+"\t"+i));
const data=readFileSync(new URL("../testdata/upstream-golden/auth-types.tsv",import.meta.url),"utf8")
 .split(/\r?\n/).filter(x=>x&&!x.startsWith("#"));
if(JSON.stringify(data)!==JSON.stringify(expected))
 throw new Error("Native auth enum fixtures disagree with pinned real GutTS source");
const types={
 luth:{
  LuthDeploymentRecord:["id","url","label","environment","isActive","isPrimary","priority"],
  LuthAuthClientConfig:["ecosystemId","applicationId","luthUrl","luthUrls","luthUrlResolver","origin","environment","autoRegister","maxRetries","retryDelayMs","timeoutMs","tokenStore"],
  LuthAuthTokenStore:["get","set","clear"],
  RegisterApplicationResult:["registered","ecosystemId","applicationId","origin","environment","authenticationEnabled"],
  LuthUser:["id","name","email","image","emailVerified"],
  LuthSession:["id","userId","expiresAt","token"],
  LuthSessionResult:["authenticated","user","session","error"],
  EmailSignInCredentials:["email","password"],
  EmailSignUpData:["email","password","name"],
  GoogleSignInOptions:["callbackURL"],
 },
 oauth:{
  OAuthClientRegistrationV1:["schemaVersion","appId","clientId","platform","publicClient","pkceMethod","redirectUris","verifiedHosts"],
 },
 capability:{
  CapabilityParameterV1:["name","kind","required"],
  CapabilityBaseV1:["schemaVersion","appId","operationId","revision","displayName","requiredScopes","inputParameters","outputParameters"],
  StaticCapabilityV1:["kind","deploymentId","sessionPermission"],
  WorkflowPublicationV1:["kind","workflowId","ownerUserId","sourceWorkspaceId","published"],
 }
};
let count=0;
for(const [file,definitions] of Object.entries(types)){
 for(const [name,fields] of Object.entries(definitions)){
  const marker="export interface "+name+" ";
  const start=files[file].indexOf(marker);
  if(start<0)throw new Error("Missing pinned type "+file+"/"+name);
  const next=files[file].indexOf("\nexport ",start+marker.length);
  const body=files[file].slice(start,next<0?undefined:next);
  for(const field of fields)
   if(!new RegExp("\\b"+field+"\\s*\\??\\s*[:(]").test(body))
    throw new Error("Pinned type "+name+" missing "+field);
  count++;
 }
}
for(const key of ["IdentityKind","IdentityId","ApplicationId","DeploymentId","OAuthClientId","UserId","WorkspaceId","UiSessionId","IdentityRef"])
 if(!new RegExp("export type\\s+"+key+"(?:\\s|<|=)").test(files.identity))throw new Error("Missing identity type "+key);
if(!files.luth.includes("export class LuthAuthError"))
 throw new Error("Missing LuthAuthError");
if(!files.oauth.includes("export type ContractCheck"))
 throw new Error("Missing ContractCheck");
if(!files.capability.includes("export type CapabilityDescriptorV1"))
 throw new Error("Missing CapabilityDescriptorV1");
console.log("Pinned GutTS auth type schemas passed: "+count+" interfaces, "+data.length+" enum literals, identity tagged types");
