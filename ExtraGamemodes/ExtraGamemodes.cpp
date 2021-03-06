// Andrew Pratt 2021
// ExtraGamemodes

#include <iostream>
#include <iomanip>
#include <windows.h>
#include <stdio.h>
#include <fstream>
#include <string>
#include <sstream>
#include <random>
#include <filesystem>
#include "nlohmann/json.hpp"
#include "fmt/core.h"


/**
 * Opens a file and declares a std::ifstream for it. Program ends if the file can't be opened
 *
 * Author: Andrew Pratt
 * Param identifier: Identifier to declare
 * Param file: Path to file, including extension (ex. "Example/Path/cool_file.txt")
 */
#define DECLARE_IFILE_CRUCIAL(identifier, file) std::ifstream identifier(file); if (!identifier.is_open()) return bailProgram("Couldn't find file {}", file);


/**
 * Ends the program and displays an error message to std::cerr
 *
 * Author: Andrew Pratt
 * Param msg: Format string to be printed
 * Param args: Arguments to be formatted
 * Returns: Exit code for main() to return
 */
int bailProgram(std::string msg);
template<class... Args>
int bailProgram(std::string msg, Args&... args);

 /**
  * Opens and parses a json file.
  *
  * Author: Andrew Pratt
  * Param in: Path of json file
  * Param out: Object to hold parsed json
  * Returns: True on success, false on failure
  */
bool parseJson(std::string in, nlohmann::json& out);


/**
 * Gets an element from a json object
 *
 * Author: Andrew Pratt
 * Param in: Path of json file
 * Param out: Object to hold parsed json
 * Returns: True on success, false on failure
 */
bool parseJson(std::string in, nlohmann::json & out);


/**
 * Create a new version 4 UUID
 *
 * Author: Andrew Pratt
 * Returns: UUID as a string
 */
std::string genV4Uuid();


// TODO: More gamemodes
// TODO: Easier way to pick base mission
// TODO: Configurable missions
// TODO: Gui?
// TODO: Use rpkg.exe to output .rpkg instead of .json
int main()
{
	try
	{
		// Get the file name of this process
		wchar_t processFileName[MAX_PATH];
		GetModuleFileName(nullptr, processFileName, _countof(processFileName));
		
		// Get the path of the json folder
		std::filesystem::path jsonPath{ processFileName };
		jsonPath = jsonPath.parent_path() / "json";

		const std::string MISSION_INFO_JSON_PATH{ (jsonPath / "mission_info.json").string() };
		const std::string OG_MISSION_PATH{ (jsonPath / "og_missions").string() };
		const std::string NPCS_PATH{ (jsonPath / "npcs").string()};


		// Grab missions info
		nlohmann::json missionsInfo;
		if (!parseJson(MISSION_INFO_JSON_PATH, missionsInfo)) return bailProgram("Failed parsing mission info");


		// Print all missions
		std::ostringstream missionInfoStream;
		for (auto& item : missionsInfo.items())
		{
			std::string missionHash{item.value().value<std::string>("hash", "") };
			if (missionHash.empty()) bailProgram(fmt::format("Missing hash for mission named \"{}\"", item.key()));

			missionInfoStream << std::left << std::setw(30) << item.key() << ": " << std::right
				<< item.value().value<std::string>("location", "(Unnamed Location)") << ", "
				<< '\"' << item.value().value<std::string>("niceName", "(Unnamed Mission)") << '\"'
				<< std::endl;
		}
		std::string missionInfoStr{ missionInfoStream.str() };

		
		// Get mission from user
		nlohmann::json missionInfo;
		bool bNeedMissionInput{true};
		std::string missionName;
		while (bNeedMissionInput)
		{
			std::cout << missionInfoStr << std::endl << std::endl << "Enter base mission: ";
			std::getline(std::cin, missionName);

			if ((missionInfo = missionsInfo[missionName]).is_null())
				std::cout << "No mission named \"" << missionName << "\" exists" << std::endl;
			else
				bNeedMissionInput = false;
		}



		// Get original mission
		std::string missionJsonPath{ fmt::format(
			"{}/{}.JSON",
			OG_MISSION_PATH,
			missionName
		) };
		nlohmann::json missionJson;
		if (!parseJson(missionJsonPath, missionJson)) return bailProgram("Failed parsing original mission json");

		// Get npc list
		std::string npcsJsonPath{ fmt::format(
			"{}/{}.JSON",
			NPCS_PATH,
			missionName
		) };
		nlohmann::json npcsJson;
		if (!parseJson(npcsJsonPath, npcsJson)) return bailProgram("Failed parsing npc list");
		if (!npcsJson.is_array()) return bailProgram("Npc list is not an array");
		
		// Get objective json
		// TODO: Implement multiple gamemodes, don't hardcode filename
		// TODO: Create a gamemode class
		nlohmann::json objectiveJson;
		if (!parseJson("json/objectives/KillEveryone.JSON", objectiveJson)) return bailProgram("Failed parsing objective json");



		// Setup objective
		// TODO: Implement this in a method of a gamemode subclass
		objectiveJson["Id"] = genV4Uuid();
		objectiveJson["Definition"]["Context"]["TargetsKilledGoal"] = npcsJson.size();
		objectiveJson["Definition"]["Context"]["TargetsLeft"] = npcsJson.size();
		nlohmann::json& objectiveTargets = objectiveJson["Definition"]["Context"]["Targets"];
		for (auto& item : npcsJson.items())
			objectiveTargets.push_back(item.value()["id"]);



		// Replace normal mission objectives with new objective
		missionJson["Data"]["Objectives"].clear();
		missionJson["Data"]["Objectives"].push_back(objectiveJson);


		// Create output file
		// TODO: Don't hardcode output directory
		// TODO: Make rpkg file instead
		std::filesystem::create_directory("chunk0patch2");
		std::string oFilePath{fmt::format("chunk0patch2/{}.JSON", missionInfo["hash"].get<std::string>())};
		std::ofstream oFile(oFilePath);
		if (!oFile.is_open()) return bailProgram(fmt::format("Failed to create {}", oFilePath));

		// Write to output file
		oFile << missionJson;
		
		// Close output file
		oFile.close();

		
		std::cout << std::endl << "Created mission json file " << oFilePath << std::endl;
		return EXIT_SUCCESS;

	}
	catch (nlohmann::json::exception e)
	{
		std::cerr << fmt::format("Json error: {}", e.what()) << std::endl;
	}
	catch (std::exception e)
	{
		std::cerr << fmt::format("Error: {}", e.what()) << std::endl;
	}
}

int bailProgram(std::string msg)
{
	std::cerr << msg << std::endl;

	return EXIT_FAILURE;
}

template<class ...Args>
int bailProgram(std::string msg, Args & ...args)
{
	return bailProgram(fmt::format(msg, args...));
}


bool parseJson(std::string in, nlohmann::json& out)
{
	try
	{
		// Open file
		std::ifstream iFile(in);

		// Bail if file can't be opened
		if (!iFile.is_open())
		{
			std::cerr << fmt::format("Couldn\'t open file {}", in) << std::endl;
			return false;
		}

		// Read into json object
		iFile >> out;

		// Close file
		iFile.close();
	}
	catch (std::exception e)
	{
		std::cerr << fmt::format("Error while trying to read file \"{}\": {}", in, e.what()) << std::endl;
	}

	return true;
}


// TODO: Make this a seperate class
std::string genV4Uuid()
{
	std::mt19937_64 engine{};
	std::uniform_int_distribution<uint64_t> dist{};

	uint64_t high	= dist(engine);
	uint64_t low	= dist(engine);

	std::stringstream idStream;
	idStream
		<< std::setfill('0') << std::setw(16) << std::hex 
			<< high
		<< std::setfill('0') << std::setw(16) << std::hex
			<< low;

	std::string idStr{ idStream.str() };

	idStr.insert(8,	 1, '-');
	idStr.insert(13, 1, '-');
	idStr.insert(18, 1, '-');
	idStr.insert(23, 1, '-');

	return idStr;
}