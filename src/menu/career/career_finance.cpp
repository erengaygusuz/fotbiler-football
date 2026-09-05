// Transitional compatibility translation unit for legacy build manifests.
#include "career_finance.hpp"
#include "core/career/career_finance.cpp"
#include "utils/localization.hpp"

namespace blunted {
namespace CareerFinance {

std::string GetFinancialHealthString(const CareerSave& save) {
  switch (GetFinancialHealth(save)) {
    case FinancialHealth::Elite:
      return TR("career_fin_elite");
    case FinancialHealth::Stable:
      return TR("career_fin_stable");
    case FinancialHealth::Tight:
      return TR("career_fin_tight");
    case FinancialHealth::Critical:
      return TR("career_fin_critical");
  }
  return TR("career_fin_critical");
}

}  // namespace CareerFinance
}  // namespace blunted
