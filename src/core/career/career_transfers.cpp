#include "career_transfers.hpp"

#include <algorithm>

namespace blunted {
namespace CareerTransfers {

namespace {
using CareerCommon::RandomInt;

void AddFreeAgentCandidate(CareerSave& save, const TransferMarketCandidate& candidate) {
  PlayerCareerState freeAgent;
  freeAgent.name = candidate.name;
  freeAgent.position = candidate.preferredPosition.empty() ? "CM" : candidate.preferredPosition;
  freeAgent.preferredPosition = freeAgent.position;
  freeAgent.age = candidate.age;
  freeAgent.ovr = candidate.overallRating;
  freeAgent.pot = std::min(90, freeAgent.ovr + RandomInt(2, 8));
  freeAgent.value = ComputeMarketValue(freeAgent.ovr, freeAgent.pot, freeAgent.age);
  freeAgent.wage = std::max(1500LL, freeAgent.value / 1400LL);
  freeAgent.transferStatus = TransferStatus::NONE;
  freeAgent.morale = 75;
  freeAgent.fitness = 90;
  freeAgent.matchForm = 50;
  save.freeAgents.push_back(freeAgent);
}

TransferTarget MakeTransferTarget(const TransferMarketCandidate& candidate) {
  TransferTarget target;
  target.name = candidate.name;
  target.preferredPosition = candidate.preferredPosition.empty() ? "CM" : candidate.preferredPosition;
  target.age = candidate.age;
  target.overallRating = candidate.overallRating;
  target.teamID = candidate.teamID;
  target.potentialRating =
      std::max(target.overallRating, target.overallRating + RandomInt(1, 10));
  target.value = ComputeMarketValue(target.overallRating, target.potentialRating, target.age);
  target.askingPrice = target.value + target.value * RandomInt(10, 30) / 100;
  target.wage = std::max(1500LL, target.value / 1400LL);
  target.isListed = true;
  return target;
}

}  // namespace

void RecruitFreeAgent(CareerSave& save, CareerCommon::CareerEvents& events,
                      const std::string& playerName) {
  auto it =
      std::find_if(save.freeAgents.begin(), save.freeAgents.end(),
                   [&playerName](const PlayerCareerState& p) { return p.name == playerName; });
  if (it == save.freeAgents.end())
    return;
  if (save.wageBudget < it->wage) {
    events.AddEvent("financial", "Failed to sign " + playerName + " due to wage budget limits", -1,
                    false);
    return;
  }
  save.wageBudget -= it->wage;
  save.finance.wageBudget = save.wageBudget;
  save.roster.push_back(*it);
  save.freeAgents.erase(it);
  events.AddEvent("recruitment", "Signed free agent " + playerName, 2, false);
  events.ModifyBoardConfidence(1);
}

void ReleasePlayer(CareerSave& save, CareerCommon::CareerEvents& events,
                   const std::string& playerName) {
  auto it = std::find_if(
      save.roster.begin(), save.roster.end(),
      [&playerName](const PlayerCareerState& player) { return player.name == playerName; });
  if (it == save.roster.end())
    return;

  // Severance pay: ~25 weeks of wages.
  const long long severance = it->wage * 25;
  if (save.finances.netWorth < severance) {
    events.AddEvent("financial",
                    "Cannot afford severance pay (" + std::to_string(severance) + ") to release " +
                        playerName,
                    0, false);
    return;
  }

  save.finances.netWorth -= severance;
  save.wageBudget += it->wage;
  save.finance.wageBudget = save.wageBudget;
  PlayerCareerState released = *it;
  released.transferStatus = TransferStatus::NONE;
  save.freeAgents.push_back(released);
  save.roster.erase(it);
  events.AddEvent("squad",
                  "Released player " + playerName + " (Severance: " + std::to_string(severance) +
                      ")",
                  -1, false);
  events.ModifyBoardConfidence(-1);
}

void SeedFreeAgents(CareerSave& save, ITransferMarketRepository& repository) {
  if (!save.freeAgents.empty())
    return;

  std::vector<TransferMarketCandidate> candidates;
  if (repository.LoadFreeAgentCandidates(candidates)) {
    for (const auto& candidate : candidates)
      AddFreeAgentCandidate(save, candidate);
  }

  if (!save.freeAgents.empty())
    return;

  // Fallback when the configured repository has no usable data.
  static const std::vector<std::string> lastNames = {"Smith", "Silva", "Muller", "Rossi",
                                                      "Garcia"};
  for (int i = 0; i < 5; ++i) {
    PlayerCareerState freeAgent;
    freeAgent.name = "FreeAgent " + lastNames[i];
    freeAgent.position = "CM";
    freeAgent.preferredPosition = "CM";
    freeAgent.age = 22 + i * 2;
    freeAgent.ovr = 60 + i * 2;
    freeAgent.pot = freeAgent.ovr + 5;
    freeAgent.value = ComputeMarketValue(freeAgent.ovr, freeAgent.pot, freeAgent.age);
    freeAgent.wage = 5000;
    freeAgent.transferStatus = TransferStatus::NONE;
    save.freeAgents.push_back(freeAgent);
  }
}

long long ComputeMarketValue(int overallRating, int potentialRating, int age) {
  long long ageModifier = 120;
  if (age >= 30)
    ageModifier = 85;
  else if (age <= 21)
    ageModifier = 135;
  const long long potentialModifier = 100 + std::max(0, potentialRating - overallRating);
  const long long baseValue = static_cast<long long>(overallRating) * overallRating * 4000;
  return std::max(50000LL, (baseValue * ageModifier * potentialModifier) / 12000LL);
}

void PopulateTransferMarket(std::vector<TransferTarget>& targets,
                            ITransferMarketRepository& repository) {
  if (!targets.empty())
    return;

  std::vector<TransferMarketCandidate> candidates;
  if (repository.LoadTransferCandidates(candidates)) {
    for (const auto& candidate : candidates)
      targets.push_back(MakeTransferTarget(candidate));
  }

  if (!targets.empty())
    return;

  // Fallback when the configured repository has no usable data.
  static const std::vector<std::string> lastNames = {"Silva", "Rossi", "Meyer", "Costa", "Lopez"};
  static const std::vector<std::string> positions = {"GK", "CB", "CM", "ST", "AM"};
  for (int i = 0; i < 15; ++i) {
    TransferMarketCandidate candidate;
    candidate.name = "Target " + lastNames[i % 5] + " " + std::to_string(i);
    candidate.preferredPosition = positions[i % 5];
    candidate.age = RandomInt(18, 31);
    candidate.overallRating = RandomInt(62, 84);
    candidate.teamID = 1000 + i;
    targets.push_back(MakeTransferTarget(candidate));
  }
}

TransferBid PlaceBid(CareerSave& save, CareerCommon::CareerEvents& events,
                     std::vector<TransferTarget>& targets, std::vector<TransferBid>& bids,
                     const std::string& playerName, long long bidAmount, int offeredWage,
                     int contractYears) {
  TransferBid bid;
  bid.playerName = playerName;
  bid.bidAmount = bidAmount;
  bid.offeredWage = offeredWage;
  bid.contractYears = contractYears;
  bid.agentFee = std::max(25000LL, bidAmount / 20);
  bid.status = BidStatus::REJECTED;

  auto targetIt = std::find_if(
      targets.begin(), targets.end(),
      [&playerName](const TransferTarget& target) { return target.name == playerName; });
  if (targetIt == targets.end())
    return bid;

  const long long totalCost = bidAmount + bid.agentFee;
  if (totalCost > save.transferBudget || offeredWage > save.wageBudget) {
    events.AddEvent("transfer", "Bid rejected for " + playerName + " due to budget limits", -1,
                    false);
    return bid;
  }

  bid.status = BidStatus::PENDING;
  bids.push_back(bid);
  events.AddEvent("transfer", "Placed bid for " + playerName, 0, false);
  return bid;
}

void WithdrawBid(std::vector<TransferBid>& bids, const std::string& playerName) {
  auto it = std::find_if(bids.begin(), bids.end(), [&playerName](const TransferBid& bid) {
    return bid.playerName == playerName;
  });
  if (it == bids.end())
    return;
  it->status = BidStatus::WITHDRAWN;
}

bool ImprovePendingBid(TransferBid& bid, long long transferBudget) {
  if (bid.status != BidStatus::PENDING)
    return false;

  const long long increase = std::max(50000LL, bid.bidAmount / 10);
  const long long proposedBid = bid.bidAmount + increase;
  const long long proposedAgentFee = std::max(25000LL, proposedBid / 20);
  if (proposedBid + proposedAgentFee > transferBudget)
    return false;

  bid.bidAmount = proposedBid;
  bid.agentFee = proposedAgentFee;
  bid.negotiationRounds++;
  return true;
}

void ProcessPendingBids(CareerSave& save, CareerCommon::CareerEvents& events,
                        std::vector<TransferTarget>& targets, std::vector<TransferBid>& bids) {
  for (auto& bid : bids) {
    if (bid.status != BidStatus::PENDING)
      continue;

    auto targetIt = std::find_if(
        targets.begin(), targets.end(),
        [&bid](const TransferTarget& target) { return target.name == bid.playerName; });
    if (targetIt == targets.end()) {
      bid.status = BidStatus::REJECTED;
      continue;
    }

    long long requiredPrice = targetIt->askingPrice;
    // Negotiation rounds progressively soften the asking price (5% per round,
    // capped at 15%) rather than switching on a flat discount at round two.
    if (bid.negotiationRounds >= 1) {
      const int discount = std::min(15, 5 * bid.negotiationRounds);
      requiredPrice = requiredPrice * (100 - discount) / 100;
    }

    if (bid.bidAmount >= requiredPrice && bid.offeredWage >= targetIt->wage * 9 / 10) {
      bid.status = BidStatus::ACCEPTED;
      events.AddEvent("transfer", "Bid accepted for " + bid.playerName, 1, false);
    } else {
      bid.status = BidStatus::REJECTED;
      events.AddEvent("transfer", "Bid rejected for " + bid.playerName, -1, false);
    }
  }
}

bool CompleteTransfer(CareerSave& save, CareerCommon::CareerEvents& events,
                      std::vector<TransferTarget>& targets, std::vector<TransferBid>& bids,
                      const std::string& playerName) {
  auto bidIt = std::find_if(bids.begin(), bids.end(), [&playerName](const TransferBid& bid) {
    return bid.playerName == playerName && bid.status == BidStatus::ACCEPTED;
  });
  if (bidIt == bids.end())
    return false;

  auto targetIt = std::find_if(
      targets.begin(), targets.end(),
      [&playerName](const TransferTarget& target) { return target.name == playerName; });
  if (targetIt == targets.end())
    return false;

  const long long totalCost = bidIt->bidAmount + bidIt->agentFee;
  if (totalCost > save.transferBudget || bidIt->offeredWage > save.wageBudget)
    return false;

  PlayerCareerState player;
  player.name = targetIt->name;
  player.preferredPosition = targetIt->preferredPosition;
  player.position = targetIt->preferredPosition;
  player.ovr = targetIt->overallRating;
  player.pot = targetIt->potentialRating;
  player.age = targetIt->age;
  player.value = targetIt->value;
  player.wage = bidIt->offeredWage;
  player.contract.yearsRemaining = bidIt->contractYears;
  player.contract.transferListed = false;
  player.morale = 70;
  player.fitness = 95;
  player.matchForm = 60;

  save.transferBudget -= totalCost;
  save.wageBudget -= bidIt->offeredWage;
  save.finance.transferBudget = save.transferBudget;
  save.finance.wageBudget = save.wageBudget;
  save.finances.transferSpending += bidIt->bidAmount;
  save.roster.push_back(player);

  targets.erase(targetIt);
  bids.erase(std::remove_if(
                 bids.begin(), bids.end(),
                 [&playerName](const TransferBid& bid) { return bid.playerName == playerName; }),
             bids.end());

  events.AddEvent("transfer", "Completed transfer for " + playerName, 2, true);
  events.ModifyBoardConfidence(1);
  return true;
}

}  // namespace CareerTransfers
}  // namespace blunted
