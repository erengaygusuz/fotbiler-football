#ifndef CAREER_TRANSFERS_COMPAT_HPP
#define CAREER_TRANSFERS_COMPAT_HPP

#include <string>
#include <vector>

// Transitional compatibility facade for legacy menu/career consumers.
// New domain code should include "core/career/career_transfers.hpp" directly.
#include "core/career/career_transfers.hpp"
#include "persistence/career/sqlite_transfer_market_repository.hpp"
#include "presentation/career/career_presentation.hpp"

namespace blunted {
namespace CareerTransfers {

inline constexpr const char* kDefaultCareerDatabase = "databases/default/database.sqlite";

// Legacy no-repository entry points kept at the composition edge while callers
// migrate to explicit ITransferMarketRepository injection.
inline void PopulateTransferMarket(std::vector<TransferTarget>& targets) {
  CareerPersistenceAdapters::SqliteTransferMarketRepository repository(kDefaultCareerDatabase);
  PopulateTransferMarket(targets, repository);
}

inline void SeedFreeAgents(CareerSave& save) {
  CareerPersistenceAdapters::SqliteTransferMarketRepository repository(kDefaultCareerDatabase);
  SeedFreeAgents(save, repository);
}

inline std::string GetBidStatusString(BidStatus status) {
  return CareerPresentation::GetBidStatusString(status);
}

}  // namespace CareerTransfers
}  // namespace blunted

#endif  // CAREER_TRANSFERS_COMPAT_HPP
