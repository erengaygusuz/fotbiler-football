#ifndef CAREER_TRANSFERS_COMPAT_HPP
#define CAREER_TRANSFERS_COMPAT_HPP

#include <string>

// Transitional compatibility include for legacy menu/career consumers.
// New domain code should include "core/career/career_transfers.hpp" directly.
#include "core/career/career_transfers.hpp"

namespace blunted {
namespace CareerTransfers {

// Legacy entry points retain the old no-repository API while wiring the domain
// to the current SQLite adapter.
void PopulateTransferMarket(std::vector<TransferTarget>& targets);
void SeedFreeAgents(CareerSave& save);

// Presentation helper retained until menu screens own all localization.
std::string GetBidStatusString(BidStatus status);

}  // namespace CareerTransfers
}  // namespace blunted

#endif  // CAREER_TRANSFERS_COMPAT_HPP
