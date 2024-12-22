#pragma once

using namespace sc2;

std::string kDefaultMap = "BelshirVestigeLE.SC2Map";

static Difficulty GetDifficultyFromString(const std::string &InDifficulty) {
    if(InDifficulty == "VeryEasy") { return Difficulty::VeryEasy; }
    if(InDifficulty == "Easy") { return Difficulty::Easy; }
    if(InDifficulty == "Medium") { return Difficulty::Medium; }
    if(InDifficulty == "MediumHard") { return Difficulty::MediumHard; }
    if(InDifficulty == "Hard") { return Difficulty::Hard; }
    if(InDifficulty == "HardVeryHard") { return Difficulty::HardVeryHard; }
    if(InDifficulty == "VeryHard") { return Difficulty::VeryHard; }
    if(InDifficulty == "CheatVision") { return Difficulty::CheatVision; }
    if(InDifficulty == "CheatMoney") { return Difficulty::CheatMoney; }
    if(InDifficulty == "CheatInsane") { return Difficulty::CheatInsane; }

    return Difficulty::Easy;
}

static Race GetRaceFromString(const std::string &RaceIn) {
    std::string race(RaceIn);
    std::transform(race.begin(), race.end(), race.begin(), ::tolower);

    if(race == "terran") {
        return Race::Terran;
    } else if(race == "protoss") {
        return Race::Protoss;
    } else if(race == "zerg") {
        return Race::Zerg;
    } else if(race == "random") {
        return Race::Random;
    }

    return Race::Random;
}

struct ConnectionOptions {
    int32_t GamePort;
    int32_t StartPort;
    std::string ServerAddress;
    bool ComputerOpponent;
    Difficulty ComputerDifficulty;
    Race ComputerRace;
    std::string OpponentId;
    std::string Map;
};

static void ParseArguments(int argc, char *argv[], ConnectionOptions &connect_options) {
    ArgParser arg_parser(argv[0]);
    arg_parser.AddOptions({{"-g", "--GamePort", "Port of client to connect to", false},
                           {"-o", "--StartPort", "Starting server port", false},
                           {"-l", "--LadderServer", "Ladder server address", false},
                           {"-c", "--ComputerOpponent", "If we set up a computer oppenent"},
                           {"-a", "--ComputerRace", "Race of computer oppent"},
                           {"-d", "--ComputerDifficulty", "Difficulty of computer oppenent"},
                           {
                             "-m",
                             "--Map",
                             "Map to play on against computer opponent",
                           },
                           {"-x", "--OpponentId", "PlayerId of opponent"}});
    arg_parser.Parse(argc, argv);
    std::string GamePortStr;
    if(arg_parser.Get("GamePort", GamePortStr)) {
        connect_options.GamePort = atoi(GamePortStr.c_str());
    }
    std::string StartPortStr;
    if(arg_parser.Get("StartPort", StartPortStr)) {
        connect_options.StartPort = atoi(StartPortStr.c_str());
    }
    arg_parser.Get("LadderServer", connect_options.ServerAddress);
    std::string CompOpp;
    if(arg_parser.Get("ComputerOpponent", CompOpp)) {
        connect_options.ComputerOpponent = true;
        std::string CompRace;
        if(arg_parser.Get("ComputerRace", CompRace)) {
            connect_options.ComputerRace = GetRaceFromString(CompRace);
        }
        std::string CompDiff;
        if(arg_parser.Get("ComputerDifficulty", CompDiff)) {
            connect_options.ComputerDifficulty = GetDifficultyFromString(CompDiff);
        }
        std::string map;
        if(arg_parser.Get("Map", map)) {
            connect_options.Map = map;
        } else {
            connect_options.Map = kDefaultMap;
        }
    } else {
        connect_options.ComputerOpponent = false;
    }
    arg_parser.Get("OpponentId", connect_options.OpponentId);
}

static void RunBot(int argc, char *argv[], Agent *Agent, Race race) {
    ConnectionOptions Options;
    ParseArguments(argc, argv, Options);

    Coordinator coordinator;

    int num_agents;
    if(Options.ComputerOpponent) {
        num_agents = 1;
        coordinator.SetParticipants(
          {CreateParticipant(race, Agent),
           CreateComputer(Options.ComputerRace, Options.ComputerDifficulty)});
        coordinator.LoadSettings(1, argv);
        // coordinator.SetRealtime(true);
        coordinator.LaunchStarcraft();
        coordinator.StartGame(Options.Map);
    } else {
        num_agents = 2;
        coordinator.SetParticipants({
          CreateParticipant(race, Agent),
        });
        // Start the game.
        std::cout << "Connecting to port " << Options.GamePort << std::endl;
        coordinator.Connect(Options.GamePort);
        coordinator.SetupPorts(num_agents, Options.StartPort, false);
        // Step forward the game simulation.
        coordinator.JoinGame();
        std::cout << " Successfully joined game" << std::endl;
    }

    coordinator.SetTimeoutMS(10000);
    while(coordinator.Update()) {}
}
