#ifndef CAREER_FINANCE_COMPAT_HPP
#define CAREER_FINANCE_COMPAT_HPP

#include <string>

// Transitional compatibility include for legacy menu/career consumers.
// New domain code should include "core/career/career_finance.hpp" directly.
#include "core/career/career_finance.hpp"

namespace blunted {
namespace CareerFinance {

// Legacy presentation helper retained while menu/career screens are migrated.
std::string GetFinancialHealthString(const CareerSave& save);

}  // namespace CareerFinance
}  // namespace blunted

#endif  // CAREER_FINANCE_COMPAT_HPP
