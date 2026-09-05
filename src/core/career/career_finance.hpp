#ifndef CAREER_FINANCE_CORE_HPP
#define CAREER_FINANCE_CORE_HPP

#include <string>

#include "../../data/careerdata.hpp"
#include "career_common.hpp"

namespace blunted {
namespace CareerFinance {

enum class FinancialHealth {
  Elite,
  Stable,
  Tight,
  Critical,
};

// Adjusts the transfer and wage budgets.
void ModifyBudget(CareerSave& save, long long transferDelta, long long wageDelta);

// Recomputes the owner-mode derived finance fields from the current state.
void InitializeOwnerData(CareerSave& save);

void SetTicketPrice(CareerSave& save, int price);
void RepairStadium(CareerSave& save, int amount);
void UpgradeStadium(CareerSave& save, CareerCommon::CareerEvents& events, int upgradeIndex);
void RenameStadium(CareerSave& save, const std::string& newName);

void InvestInFanBase(CareerSave& save, long long amount);
void InvestInPrestige(CareerSave& save, long long amount);

// Closes out the season's finances: revenue, expenses, and budget rollover.
void ProcessSeasonFinances(CareerSave& save);

long long GetSeasonProfit(const CareerSave& save);

// Domain-level financial health classification. Presentation layers are
// responsible for converting this value to localized display text.
FinancialHealth GetFinancialHealth(const CareerSave& save);

}  // namespace CareerFinance
}  // namespace blunted

#endif  // CAREER_FINANCE_CORE_HPP
