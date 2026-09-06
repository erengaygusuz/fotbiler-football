#ifndef CAREER_PRESENTATION_HPP
#define CAREER_PRESENTATION_HPP

#include <string>
#include <vector>

#include "../../data/careerdata.hpp"
#include "core/career/career_finance.hpp"
#include "utils/localization.hpp"

namespace blunted::CareerPresentation {

inline std::string GetReputationStatus(const ::CareerSave* save) {
  if (!save)
    return TR("career_rep_unknown");
  const int reputation = save->reputation;
  if (reputation >= 80)
    return TR("career_rep_legendary");
  if (reputation >= 50)
    return TR("career_rep_respected");
  if (reputation >= 20)
    return TR("career_rep_known");
  if (reputation >= -20)
    return TR("career_rep_unproven");
  if (reputation >= -50)
    return TR("career_rep_controversial");
  return TR("career_rep_notorious");
}

inline std::string GetMoraleString(int morale) {
  if (morale >= 80)
    return TR("career_morale_happy");
  if (morale >= 40)
    return TR("career_morale_content");
  return TR("career_morale_unhappy");
}

inline std::string GetFormString(int form) {
  if (form >= 80)
    return TR("career_form_excellent");
  if (form >= 40)
    return TR("career_form_good");
  return TR("career_form_poor");
}

inline std::string GetConditionArrow(int form) {
  if (form >= 85)
    return "[^] TOP";
  if (form >= 65)
    return "[/] GOOD";
  if (form >= 40)
    return "[>] NORM";
  if (form >= 20)
    return "[\\] POOR";
  return "[v] BAD";
}

inline std::string GetFinancialHealthString(const ::CareerSave* save) {
  if (!save)
    return "Unknown";

  switch (CareerFinance::GetFinancialHealth(*save)) {
    case CareerFinance::FinancialHealth::Elite:
      return TR("career_fin_elite");
    case CareerFinance::FinancialHealth::Stable:
      return TR("career_fin_stable");
    case CareerFinance::FinancialHealth::Tight:
      return TR("career_fin_tight");
    case CareerFinance::FinancialHealth::Critical:
      return TR("career_fin_critical");
  }
  return TR("career_fin_critical");
}

inline std::string GetBidStatusString(BidStatus status) {
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

inline std::string GetFormGuideString(const ::CareerSave* save, int count) {
  if (!save)
    return "[ - - - - - ]";

  const int wins = save->seasonWins;
  const int draws = save->seasonDraws;
  const int losses = save->seasonLosses;
  const int total = wins + draws + losses;
  if (total == 0)
    return "[ - - - - - ]";

  std::string guide;
  for (int i = 0; i < count; ++i) {
    const int seed = (save->season.currentWeek * 7 + i * 13) % 100;
    if (wins > 0 && seed < (wins * 100 / total)) {
      guide += "[W] ";
    } else if (draws > 0 && seed < ((wins + draws) * 100 / total)) {
      guide += "[D] ";
    } else {
      guide += "[L] ";
    }
  }
  return guide;
}

inline std::vector<std::string> GetNewsHeadlines(const ::CareerSave* save, int count) {
  if (!save)
    return {"Transfer window opens with record activity across top divisions."};

  std::vector<std::string> headlines;
  const int played = save->seasonWins + save->seasonDraws + save->seasonLosses;
  if (played == 0) {
    headlines.push_back("PRE-SEASON: " + save->name + " gears up for ambitious campaign in " +
                        save->club.leagueName + ".");
  } else if (save->seasonWins > save->seasonLosses * 2) {
    headlines.push_back("MEDIA SPOTLIGHT: Pundits praise " + save->name +
                        "'s tactical fluidity and dominant run of form.");
  } else if (save->seasonLosses > save->seasonWins) {
    headlines.push_back("PRESSURE BUILDS: Manager " + save->managerName +
                        " calls for resilience amid testing fixture schedule.");
  } else {
    headlines.push_back("COMPETITIVE RACE: " + save->name +
                        " stays in contention as mid-table battle intensifies.");
  }

  if (!save->youthAcademy.empty()) {
    headlines.push_back("ACADEMY REPORT: Scouts spotlight " + save->youthAcademy[0].name +
                        " as a potential future star.");
  } else if (!save->roster.empty()) {
    headlines.push_back("SQUAD FOCUS: " + save->roster[0].name +
                        " maintaining peak match condition ahead of next clash.");
  }

  if (save->boardConfidence >= 75) {
    headlines.push_back(
        "BOARD CONFIDENCE: Club hierarchy 'delighted' with current management and financial "
        "health.");
  } else {
    headlines.push_back(
        "BOARD NOTICE: Club leadership expects strong performance in upcoming league fixtures.");
  }

  while (static_cast<int>(headlines.size()) > count)
    headlines.pop_back();
  return headlines;
}

inline std::string GetNextOpponentPreview(int week) {
  static const std::vector<std::string> opponentNames = {
      "FC United",     "Athletic Club", "Wanderers FC",      "Real Deportivo", "Inter Milano",
      "Bayern Munich", "FC Barcelona",  "Chelsea FC",        "Arsenal FC",     "Juventus Turin",
      "AC Milan",      "Liverpool FC",  "Borussia Dortmund", "Paris SG",       "Ajax Amsterdam",
      "Porto FC",      "Benfica",       "Sporting CP",       "Napoli",         "Atletico Madrid",
      "Tottenham"};
  const int opponentIdx = (week * 3) % static_cast<int>(opponentNames.size());
  const std::string opp = opponentNames[opponentIdx];
  const bool isHome = (week % 2) == 0;
  const std::string venue = isHome ? "Home (Your Stadium)" : "Away (" + opp + " Arena)";
  const std::string danger =
      ((week % 3) == 0)
          ? "★★★★★ High Danger"
          : (((week % 2) == 0) ? "★★★☆☆ Moderate Threat" : "★★★★☆ Solid Defense");
  return opp + " | Venue: " + venue + "\nThreat Rating: " + danger +
         " | Expected Strategy: Balanced Press";
}

}  // namespace blunted::CareerPresentation

#endif  // CAREER_PRESENTATION_HPP
