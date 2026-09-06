#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "core/career/career_simulation_scenarios.hpp"

namespace fs = std::filesystem;

namespace {

struct Options {
  int runs = 1000;
  bool list = false;
  bool console = true;
  bool seedOverrideSet = false;
  unsigned int seedOverride = 0;
  std::string scenarioName;
  fs::path csvPath;
  fs::path jsonPath;
  fs::path baselinePath;
};

struct BaselineMetrics {
  double avgUserGoals = 0.0;
  double avgOpponentGoals = 0.0;
  double avgGoalDifference = 0.0;
  double winRate = 0.0;
  double avgUserPossession = 0.0;
};

void PrintUsage(const char* argv0) {
  std::cout
      << "Fotbiler gameplay simulation runner\n\n"
      << "Usage:\n  " << argv0 << " [options]\n\n"
      << "Options:\n"
      << "  --runs N             Matches per selected scenario (default: 1000)\n"
      << "  --scenario NAME      Run one scenario instead of the whole catalogue\n"
      << "  --seed N             Override every selected scenario seed\n"
      << "  --list               List scenario names and exit\n"
      << "  --csv PATH           CSV output path\n"
      << "  --json PATH          JSON output path\n"
      << "  --baseline PATH      Compare console metrics with a previous CSV report\n"
      << "  --no-console         Suppress per-scenario console summary\n"
      << "  -h, --help           Show this help\n\n"
      << "Without --csv/--json, reports are written beside the executable under\n"
      << "simulation-reports/latest.csv and simulation-reports/latest.json.\n";
}

int ParsePositiveInt(const std::string& value, const char* option) {
  try {
    size_t consumed = 0;
    const int parsed = std::stoi(value, &consumed);
    if (consumed != value.size() || parsed <= 0)
      throw std::invalid_argument("not positive");
    return parsed;
  } catch (const std::exception&) {
    throw std::runtime_error(std::string(option) + " expects a positive integer: " + value);
  }
}

unsigned int ParseUnsigned(const std::string& value, const char* option) {
  try {
    size_t consumed = 0;
    const unsigned long parsed = std::stoul(value, &consumed);
    if (consumed != value.size())
      throw std::invalid_argument("trailing characters");
    return static_cast<unsigned int>(parsed);
  } catch (const std::exception&) {
    throw std::runtime_error(std::string(option) + " expects an unsigned integer: " + value);
  }
}

Options ParseOptions(int argc, char** argv) {
  Options options;

  const fs::path executablePath = fs::absolute(argv[0]).parent_path();
  const fs::path reportDir = executablePath / "simulation-reports";
  options.csvPath = reportDir / "latest.csv";
  options.jsonPath = reportDir / "latest.json";

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    auto requireValue = [&](const char* option) -> std::string {
      if (i + 1 >= argc)
        throw std::runtime_error(std::string(option) + " requires a value");
      return argv[++i];
    };

    if (arg == "--runs") {
      options.runs = ParsePositiveInt(requireValue("--runs"), "--runs");
    } else if (arg == "--scenario") {
      options.scenarioName = requireValue("--scenario");
    } else if (arg == "--seed") {
      options.seedOverride = ParseUnsigned(requireValue("--seed"), "--seed");
      options.seedOverrideSet = true;
    } else if (arg == "--csv") {
      options.csvPath = requireValue("--csv");
    } else if (arg == "--json") {
      options.jsonPath = requireValue("--json");
    } else if (arg == "--baseline") {
      options.baselinePath = requireValue("--baseline");
    } else if (arg == "--list") {
      options.list = true;
    } else if (arg == "--no-console") {
      options.console = false;
    } else if (arg == "-h" || arg == "--help") {
      PrintUsage(argv[0]);
      std::exit(0);
    } else {
      throw std::runtime_error("Unknown option: " + arg);
    }
  }

  return options;
}

std::string CsvEscape(const std::string& value) {
  if (value.find_first_of(",\"\n\r") == std::string::npos)
    return value;
  std::string out = "\"";
  for (char c : value) {
    if (c == '\"')
      out += "\"\"";
    else
      out += c;
  }
  out += "\"";
  return out;
}

std::string JsonEscape(const std::string& value) {
  std::ostringstream out;
  for (unsigned char c : value) {
    switch (c) {
      case '\"': out << "\\\""; break;
      case '\\': out << "\\\\"; break;
      case '\b': out << "\\b"; break;
      case '\f': out << "\\f"; break;
      case '\n': out << "\\n"; break;
      case '\r': out << "\\r"; break;
      case '\t': out << "\\t"; break;
      default:
        if (c < 0x20) {
          out << "\\u" << std::hex << std::setw(4) << std::setfill('0')
              << static_cast<int>(c) << std::dec << std::setfill(' ');
        } else {
          out << static_cast<char>(c);
        }
    }
  }
  return out.str();
}

void EnsureParentDirectory(const fs::path& path) {
  if (!path.parent_path().empty())
    fs::create_directories(path.parent_path());
}

std::pair<std::string, long long> TopScorer(
    const blunted::MatchSimulationTelemetry& telemetry) {
  std::pair<std::string, long long> top{"", 0};
  for (const auto& entry : telemetry.userScorerGoals) {
    if (entry.second > top.second ||
        (entry.second == top.second && !entry.first.empty() && entry.first < top.first)) {
      top = entry;
    }
  }
  return top;
}

void WriteCsv(const fs::path& path,
              const std::vector<blunted::CareerSimulationScenarioReport>& reports) {
  EnsureParentDirectory(path);
  std::ofstream out(path);
  if (!out)
    throw std::runtime_error("Could not open CSV output: " + path.string());

  out << "scenario,seed,runs,roster_overall,roster_size,morale,match_form,fitness,strategy,"
         "opponent_rating,user_is_home,completed_runs,interactive_runs,wins,draws,losses,"
         "win_rate,draw_rate,loss_rate,avg_user_goals,avg_opponent_goals,avg_total_goals,"
         "avg_goal_difference,avg_user_shots,avg_opponent_shots,user_shot_conversion,"
         "opponent_shot_conversion,avg_user_possession,top_scorer,top_scorer_goals\n";

  out << std::fixed << std::setprecision(6);
  for (const auto& report : reports) {
    const auto& s = report.scenario;
    const auto& t = report.telemetry;
    const auto top = TopScorer(t);
    out << CsvEscape(s.name) << ',' << s.seed << ',' << s.runs << ',' << s.rosterOverall << ','
        << s.rosterSize << ',' << s.morale << ',' << s.matchForm << ',' << s.fitness << ','
        << CsvEscape(s.strategy) << ',' << s.opponentRating << ',' << (s.userIsHome ? 1 : 0)
        << ',' << t.completedRuns << ',' << t.interactiveRuns << ',' << t.wins << ',' << t.draws
        << ',' << t.losses << ',' << t.WinRate() << ',' << t.DrawRate() << ',' << t.LossRate()
        << ',' << t.AverageUserGoals() << ',' << t.AverageOpponentGoals() << ','
        << t.AverageTotalGoals() << ',' << t.AverageGoalDifference() << ',' << t.AverageUserShots()
        << ',' << t.AverageOpponentShots() << ',' << t.UserShotConversion() << ','
        << t.OpponentShotConversion() << ',' << t.AverageUserPossession() << ','
        << CsvEscape(top.first) << ',' << top.second << '\n';
  }
}

void WriteJson(const fs::path& path,
               const std::vector<blunted::CareerSimulationScenarioReport>& reports) {
  EnsureParentDirectory(path);
  std::ofstream out(path);
  if (!out)
    throw std::runtime_error("Could not open JSON output: " + path.string());

  long long totalRequested = 0;
  long long totalCompleted = 0;
  for (const auto& report : reports) {
    totalRequested += report.telemetry.requestedRuns;
    totalCompleted += report.telemetry.completedRuns;
  }

  out << std::fixed << std::setprecision(6);
  out << "{\n  \"schema_version\": 1,\n"
      << "  \"scenario_count\": " << reports.size() << ",\n"
      << "  \"requested_matches\": " << totalRequested << ",\n"
      << "  \"completed_matches\": " << totalCompleted << ",\n"
      << "  \"scenarios\": [\n";

  for (size_t i = 0; i < reports.size(); ++i) {
    const auto& report = reports[i];
    const auto& s = report.scenario;
    const auto& t = report.telemetry;
    out << "    {\n"
        << "      \"name\": \"" << JsonEscape(s.name) << "\",\n"
        << "      \"seed\": " << s.seed << ",\n"
        << "      \"runs\": " << s.runs << ",\n"
        << "      \"inputs\": {\n"
        << "        \"roster_overall\": " << s.rosterOverall << ",\n"
        << "        \"roster_size\": " << s.rosterSize << ",\n"
        << "        \"morale\": " << s.morale << ",\n"
        << "        \"match_form\": " << s.matchForm << ",\n"
        << "        \"fitness\": " << s.fitness << ",\n"
        << "        \"strategy\": \"" << JsonEscape(s.strategy) << "\",\n"
        << "        \"opponent_rating\": " << s.opponentRating << ",\n"
        << "        \"user_is_home\": " << (s.userIsHome ? "true" : "false") << "\n"
        << "      },\n"
        << "      \"metrics\": {\n"
        << "        \"completed_runs\": " << t.completedRuns << ",\n"
        << "        \"interactive_runs\": " << t.interactiveRuns << ",\n"
        << "        \"wins\": " << t.wins << ",\n"
        << "        \"draws\": " << t.draws << ",\n"
        << "        \"losses\": " << t.losses << ",\n"
        << "        \"win_rate\": " << t.WinRate() << ",\n"
        << "        \"draw_rate\": " << t.DrawRate() << ",\n"
        << "        \"loss_rate\": " << t.LossRate() << ",\n"
        << "        \"avg_user_goals\": " << t.AverageUserGoals() << ",\n"
        << "        \"avg_opponent_goals\": " << t.AverageOpponentGoals() << ",\n"
        << "        \"avg_total_goals\": " << t.AverageTotalGoals() << ",\n"
        << "        \"avg_goal_difference\": " << t.AverageGoalDifference() << ",\n"
        << "        \"avg_user_shots\": " << t.AverageUserShots() << ",\n"
        << "        \"avg_opponent_shots\": " << t.AverageOpponentShots() << ",\n"
        << "        \"user_shot_conversion\": " << t.UserShotConversion() << ",\n"
        << "        \"opponent_shot_conversion\": " << t.OpponentShotConversion() << ",\n"
        << "        \"avg_user_possession\": " << t.AverageUserPossession() << "\n"
        << "      },\n"
        << "      \"user_scorer_goals\": {";

    bool first = true;
    for (const auto& scorer : t.userScorerGoals) {
      if (!first)
        out << ',';
      out << "\n        \"" << JsonEscape(scorer.first) << "\": " << scorer.second;
      first = false;
    }
    if (!t.userScorerGoals.empty())
      out << '\n' << "      ";
    out << "}\n    }" << (i + 1 < reports.size() ? "," : "") << '\n';
  }

  out << "  ]\n}\n";
}

std::vector<std::string> ParseCsvLine(const std::string& line) {
  std::vector<std::string> fields;
  std::string current;
  bool quoted = false;
  for (size_t i = 0; i < line.size(); ++i) {
    const char c = line[i];
    if (quoted) {
      if (c == '\"' && i + 1 < line.size() && line[i + 1] == '\"') {
        current += '\"';
        ++i;
      } else if (c == '\"') {
        quoted = false;
      } else {
        current += c;
      }
    } else if (c == '\"') {
      quoted = true;
    } else if (c == ',') {
      fields.push_back(current);
      current.clear();
    } else {
      current += c;
    }
  }
  fields.push_back(current);
  return fields;
}

std::map<std::string, BaselineMetrics> ReadBaselineCsv(const fs::path& path) {
  std::ifstream in(path);
  if (!in)
    throw std::runtime_error("Could not open baseline CSV: " + path.string());

  std::string headerLine;
  if (!std::getline(in, headerLine))
    throw std::runtime_error("Baseline CSV is empty: " + path.string());
  const auto header = ParseCsvLine(headerLine);
  std::map<std::string, size_t> index;
  for (size_t i = 0; i < header.size(); ++i)
    index[header[i]] = i;

  const char* required[] = {"scenario", "avg_user_goals", "avg_opponent_goals",
                            "avg_goal_difference", "win_rate", "avg_user_possession"};
  for (const char* name : required) {
    if (index.find(name) == index.end())
      throw std::runtime_error(std::string("Baseline CSV missing column: ") + name);
  }

  std::map<std::string, BaselineMetrics> baseline;
  std::string line;
  while (std::getline(in, line)) {
    if (line.empty())
      continue;
    const auto fields = ParseCsvLine(line);
    auto value = [&](const char* name) -> const std::string& {
      const size_t i = index.at(name);
      if (i >= fields.size())
        throw std::runtime_error("Malformed baseline row: " + line);
      return fields[i];
    };
    BaselineMetrics metrics;
    metrics.avgUserGoals = std::stod(value("avg_user_goals"));
    metrics.avgOpponentGoals = std::stod(value("avg_opponent_goals"));
    metrics.avgGoalDifference = std::stod(value("avg_goal_difference"));
    metrics.winRate = std::stod(value("win_rate"));
    metrics.avgUserPossession = std::stod(value("avg_user_possession"));
    baseline[value("scenario")] = metrics;
  }
  return baseline;
}

void PrintReport(const std::vector<blunted::CareerSimulationScenarioReport>& reports) {
  long long total = 0;
  for (const auto& report : reports)
    total += report.telemetry.requestedRuns;

  std::cout << "Fotbiler Simulation Lab: " << reports.size() << " scenario(s), " << total
            << " requested matches\n";
  std::cout << std::fixed << std::setprecision(3);
  for (const auto& report : reports) {
    const auto& s = report.scenario;
    const auto& t = report.telemetry;
    std::cout << std::left << std::setw(30) << s.name << std::right
              << " seed=" << std::setw(8) << s.seed << " n=" << std::setw(6) << t.completedRuns
              << " W/D/L=" << t.wins << '/' << t.draws << '/' << t.losses
              << " goals=" << t.AverageUserGoals() << '-' << t.AverageOpponentGoals()
              << " gd=" << std::showpos << t.AverageGoalDifference() << std::noshowpos
              << " poss=" << t.AverageUserPossession() << "%\n";
  }
}

void PrintBaselineComparison(
    const std::vector<blunted::CareerSimulationScenarioReport>& reports,
    const std::map<std::string, BaselineMetrics>& baseline) {
  std::cout << "\nBaseline deltas (current - baseline)\n";
  std::cout << std::fixed << std::setprecision(3);
  for (const auto& report : reports) {
    const auto it = baseline.find(report.scenario.name);
    if (it == baseline.end()) {
      std::cout << std::left << std::setw(30) << report.scenario.name
                << " no matching baseline row\n";
      continue;
    }
    const auto& t = report.telemetry;
    const auto& b = it->second;
    std::cout << std::left << std::setw(30) << report.scenario.name << std::right
              << " dG=" << std::showpos << (t.AverageUserGoals() - b.avgUserGoals)
              << " dGA=" << (t.AverageOpponentGoals() - b.avgOpponentGoals)
              << " dGD=" << (t.AverageGoalDifference() - b.avgGoalDifference)
              << " dWin=" << (t.WinRate() - b.winRate)
              << " dPoss=" << (t.AverageUserPossession() - b.avgUserPossession)
              << std::noshowpos << '\n';
  }
}

std::vector<blunted::CareerSimulationScenario> SelectScenarios(const Options& options) {
  auto scenarios = blunted::BuildDefaultCareerSimulationScenarioCatalogue(options.runs);

  if (!options.scenarioName.empty()) {
    scenarios.erase(
        std::remove_if(scenarios.begin(), scenarios.end(), [&](const auto& scenario) {
          return scenario.name != options.scenarioName;
        }),
        scenarios.end());
    if (scenarios.empty())
      throw std::runtime_error("Unknown scenario: " + options.scenarioName);
  }

  if (options.seedOverrideSet) {
    for (auto& scenario : scenarios)
      scenario.seed = options.seedOverride;
  }
  return scenarios;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    const Options options = ParseOptions(argc, argv);
    const auto catalogue = blunted::BuildDefaultCareerSimulationScenarioCatalogue(1);

    if (options.list) {
      for (const auto& scenario : catalogue)
        std::cout << scenario.name << '\n';
      return 0;
    }

    const auto scenarios = SelectScenarios(options);
    std::vector<blunted::CareerSimulationScenarioReport> reports;
    reports.reserve(scenarios.size());
    for (const auto& scenario : scenarios)
      reports.push_back(blunted::RunCareerSimulationScenario(scenario));

    WriteCsv(options.csvPath, reports);
    WriteJson(options.jsonPath, reports);

    if (options.console)
      PrintReport(reports);

    if (!options.baselinePath.empty()) {
      const auto baseline = ReadBaselineCsv(options.baselinePath);
      PrintBaselineComparison(reports, baseline);
    }

    if (options.console) {
      std::cout << "\nCSV : " << options.csvPath << '\n'
                << "JSON: " << options.jsonPath << '\n';
    }
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "fotbiler_sim: " << e.what() << '\n';
    return 2;
  }
}
