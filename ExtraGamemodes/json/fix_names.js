var fs = require("fs");

var repoStr = fs.readFileSync("00204D1AFD76AB13.REPO.json");
var repo = JSON.parse(repoStr);
var missionInfoStr = fs.readFileSync("mission_info.json");
var missionInfo = JSON.parse(missionInfoStr);

function getRepoName(id)
{
	for ([k, item] of Object.entries(repo))
	{	
		if (item.ID_ === id) return item.Name;
	}
	console.log("Failed to find name of " + id);
	return "ERROR";
}

for ([k,v] of Object.entries(missionInfo))
{
	var npcsPath = "npcs/" + k + ".JSON"
	var npcsStr = fs.readFileSync(npcsPath);
	console.log("Doing " + npcsPath);
	var npcs = JSON.parse(npcsStr);
	
	for ([i,npc] of Object.entries(npcs))
	{
		npc["name"] = getRepoName(npc["id"].toLowerCase());
	}
	
	fs.writeFile(npcsPath, JSON.stringify(npcs), (err, fd) =>
	{
		if (err)
		{
			console.log("Error occured while writing to file");
		}
	});
}

console.log("Done!");