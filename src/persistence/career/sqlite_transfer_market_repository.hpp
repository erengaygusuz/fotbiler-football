#ifndef SQLITE_TRANSFER_MARKET_REPOSITORY_HPP
#define SQLITE_TRANSFER_MARKET_REPOSITORY_HPP

#include <string>

#include "core/career/career_transfers.hpp"

namespace blunted {
namespace CareerPersistenceAdapters {

class SqliteTransferMarketRepository final : public CareerTransfers::ITransferMarketRepository {
public:
  explicit SqliteTransferMarketRepository(std::string databasePath);

  bool LoadFreeAgentCandidates(
      std::vector<CareerTransfers::TransferMarketCandidate>& candidates) override;
  bool LoadTransferCandidates(
      std::vector<CareerTransfers::TransferMarketCandidate>& candidates) override;

private:
  std::string databasePath;
};

}  // namespace CareerPersistenceAdapters
}  // namespace blunted

#endif  // SQLITE_TRANSFER_MARKET_REPOSITORY_HPP
