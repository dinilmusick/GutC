import {readFileSync} from "node:fs";
import {resolve} from "node:path";
import {pathToFileURL} from "node:url";
const dir=process.env.GUTTS_DIST;
if(!dir)throw new Error("GUTTS_DIST is required");
const registry=await import(pathToFileURL(resolve(dir,"api/index.js")).href);
const snapshot=JSON.parse(readFileSync(new URL("../testdata/upstream-golden/api-catalog.json",import.meta.url),"utf8"));
if(snapshot.length!==38)throw new Error("Expected 38 pinned GutTS APIs, got "+snapshot.length);
if(JSON.stringify(snapshot)!==JSON.stringify(registry.guttsApiDefinitions))
  throw new Error("C embedded catalog differs from actual pinned GutTS definitions");
for(const descriptor of snapshot) {
  const actual=registry.guttsApiById[descriptor.id];
  if(JSON.stringify(actual)!==JSON.stringify(descriptor))
    throw new Error("Registry lookup differs for "+descriptor.id);
}
const nodeMetadata=snapshot.map(registry.guttsApiToFodNode);
if(JSON.stringify(nodeMetadata)!==JSON.stringify(registry.guttsFodNodeDefinitions))
  throw new Error("Pinned FOD node registry parity failed");
console.log("Pinned GutTS 38 API definitions and FOD metadata fixture passed");
