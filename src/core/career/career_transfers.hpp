#ifndef CAREER_TRANSFERS_CORE_HPP
#define CAREER_TRANSFERS_CORE_HPP

#include <string>
#include <vector>

#include "../../data/careerdata.hpp"
#include "career_common.hpp"

namespace blunted {
namespace CareerTransfers {

// Persistence-neutral row used to seed transfer-domain objects. Repository
// adapters load only source data; valuation, potential, wages, and negotiation
// rules remain in the career domain.
struct TransferMarketCandidate {
  std::string name;
  std::string preferredPosition;
  int age = 0;
  int overallRating = 0;
  int teamID = -1;
};

class ITransferMarketRepository {
public:
  virtual ~ITransferMarketRepository() = default;

  virtual bool LoadFreeAgentCandidates(std::vector<TransferMarketCandidate>& candidates) = 0;
  virtual bool LoadTransferCandidates(std::vector<TransferMarketCandidate>& candidates) = 0;
};

// Signs a player from the free-agent pool into the roster (budget permitting).
void RecruitFreeAgent(CareerSave& save, CareerCommon::CareerEvents& events,
                      const std::string& playerName);

// Age- and potential-aware market valuation used for transfer targets: young
// high-ceiling players carry a premium, veterans are discounted. Mirrors
// CareerSim::UpdatePlayerValue so market and roster values stay consistent.
long long ComputeMarketValue(int overallRating, int potentialRating, int age);

// Releases a roster player into the free-agent pool.
void ReleasePlayer(CareerSave& save, CareerCommon::CareerEvents& events,
                   const std::string& playerName);

// Populates market data through a persistence-neutral repository. If the
// repository has no usable data, deterministic fallback content is generated.
void PopulateTransferMarket(std::vector<TransferTarget>& targets,
                            ITransferMarketRepository& repository);
void SeedFreeAgents(CareerSave& save, ITransferMarketRepository& repository);

// Places a bid on a listed target; returns the resulting bid object.
TransferBid PlaceBid(CareerSave& save, CareerCommon::CareerEvents& events,
                     std::vector<TransferTarget>& targets, std::vector<TransferBid>& bids,
                     const std::string& playerName, long long bidAmount, int offeredWage,
                     int contractYears);

void WithdrawBid(std::vector<TransferBid>& bids, const std::string& playerName);

// Raises a pending bid by 10% (at least EUR 50k) when the improved offer and
// agent fee remain affordable. Returns true only when the bid was changed.
bool ImprovePendingBid(TransferBid& bid, long long transferBudget);

// Resolves all pending bids against their targets' asking prices.
void ProcessPendingBids(CareerSave& save, CareerCommon::CareerEvents& events,
                        std::vector<TransferTarget>& targets, std::vector<TransferBid>& bids);

// Completes an accepted bid: moves the target into the roster and settles
// budgets. Returns false if no accepted bid / target matches.
bool CompleteTransfer(CareerSave& save, CareerCommon::CareerEvents& events,
                      std::vector<TransferTarget>& targets, std::vector<TransferBid>& bids,
                      const std::string& playerName);

}  // namespace CareerTransfers
}  // namespace blunted

#endif  // CAREER_TRANSFERS_CORE_HPP
