#include "sqlite_transfer_market_repository.hpp"

#include <sqlite3.h>

#include <utility>

namespace blunted {
namespace CareerPersistenceAdapters {

namespace {

std::string ReadText(sqlite3_stmt* statement, int column) {
  const char* raw = reinterpret_cast<const char*>(sqlite3_column_text(statement, column));
  return raw ? raw : "";
}

std::string ComposeName(sqlite3_stmt* statement) {
  const std::string first = ReadText(statement, 0);
  const std::string last = ReadText(statement, 1);
  return first.empty() ? last : first + " " + last;
}

}  // namespace

SqliteTransferMarketRepository::SqliteTransferMarketRepository(std::string databasePath)
    : databasePath(std::move(databasePath)) {}

bool SqliteTransferMarketRepository::LoadFreeAgentCandidates(
    std::vector<CareerTransfers::TransferMarketCandidate>& candidates) {
  candidates.clear();

  sqlite3* database = nullptr;
  if (sqlite3_open(databasePath.c_str(), &database) != SQLITE_OK) {
    if (database)
      sqlite3_close(database);
    return false;
  }

  constexpr const char* query =
      "SELECT firstname, lastname, role, age, base_stat FROM players "
      "WHERE base_stat <= 70 ORDER BY RANDOM() LIMIT 10";
  sqlite3_stmt* statement = nullptr;
  if (sqlite3_prepare_v2(database, query, -1, &statement, nullptr) != SQLITE_OK) {
    sqlite3_close(database);
    return false;
  }

  while (sqlite3_step(statement) == SQLITE_ROW) {
    CareerTransfers::TransferMarketCandidate candidate;
    candidate.name = ComposeName(statement);
    candidate.preferredPosition = ReadText(statement, 2);
    if (candidate.preferredPosition.empty())
      candidate.preferredPosition = "CM";
    candidate.age = sqlite3_column_int(statement, 3);
    candidate.overallRating = sqlite3_column_int(statement, 4);
    candidates.push_back(candidate);
  }

  sqlite3_finalize(statement);
  sqlite3_close(database);
  return !candidates.empty();
}

bool SqliteTransferMarketRepository::LoadTransferCandidates(
    std::vector<CareerTransfers::TransferMarketCandidate>& candidates) {
  candidates.clear();

  sqlite3* database = nullptr;
  if (sqlite3_open(databasePath.c_str(), &database) != SQLITE_OK) {
    if (database)
      sqlite3_close(database);
    return false;
  }

  constexpr const char* query =
      "SELECT firstname, lastname, role, age, base_stat, team_id FROM players "
      "ORDER BY RANDOM() LIMIT 20";
  sqlite3_stmt* statement = nullptr;
  if (sqlite3_prepare_v2(database, query, -1, &statement, nullptr) != SQLITE_OK) {
    sqlite3_close(database);
    return false;
  }

  while (sqlite3_step(statement) == SQLITE_ROW) {
    CareerTransfers::TransferMarketCandidate candidate;
    candidate.name = ComposeName(statement);
    candidate.preferredPosition = ReadText(statement, 2);
    if (candidate.preferredPosition.empty())
      candidate.preferredPosition = "CM";
    candidate.age = sqlite3_column_int(statement, 3);
    candidate.overallRating = sqlite3_column_int(statement, 4);
    candidate.teamID = sqlite3_column_int(statement, 5);
    candidates.push_back(candidate);
  }

  sqlite3_finalize(statement);
  sqlite3_close(database);
  return !candidates.empty();
}

}  // namespace CareerPersistenceAdapters
}  // namespace blunted
