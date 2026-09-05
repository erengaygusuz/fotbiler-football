// Transitional compatibility translation unit for the legacy menu build list.
// Canonical transfer rules live under src/core/career; SQLite access is kept
// behind a repository adapter under src/persistence/career.
#include "career_transfers.hpp"

#include "core/career/career_transfers.cpp"
#include "persistence/career/sqlite_transfer_market_repository.cpp"
#include "utils/localization.hpp"

namespace blunted {
namespace CareerTransfers {

namespace {
constexpr const char* kDefaultCareerDatabase = "databases/default/database.sqlite";
}

void PopulateTransferMarket(std::vector<TransferTarget>& targets) {
  CareerPersistenceAdapters::SqliteTransferMarketRepository repository(kDefaultCareerDatabase);
  PopulateTransferMarket(targets, repository);
}

void SeedFreeAgents(CareerSave& save) {
  CareerPersistenceAdapters::SqliteTransferMarketRepository repository(kDefaultCareerDatabase);
  SeedFreeAgents(save, repository);
}

std::string GetBidStatusString(BidStatus status) {
  switch (status) {
    case BidStatus::PENDING:
      return TR("career_bid_pending");
    case BidStatus::ACCEPTED:
      return TR("career_bid_accepted");
    case BidStatus::REJECTED:
      return TR("career_bid_rejected_status");
    case BidStatus::WITHDRAWN:
      return TR("career_bid_withdrawn");
  }
  return TR("career_bid_pending");
}

}  // namespace CareerTransfers
}  // namespace blunted
