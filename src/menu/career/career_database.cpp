#include "career_database.hpp"

#include <string>
#include <vector>

#include "application/career/career_lifecycle_service.hpp"
#include "application/career/career_save_service.hpp"
#include "core/career/career_event_service.hpp"
#include "core/career/fast_match_engine.hpp"
#include "integration/league_soccer/full_3d_match_engine.hpp"
#include "career_board.hpp"
#include "career_finance.hpp"
#include "career_sim.hpp"
#include "career_sponsors.hpp"
#include "career_staff.hpp"
#include "career_training.hpp"
#include "career_transfers.hpp"
#include "presentation/career/career_presentation.hpp"

namespace blunted {

CareerDatabase::CareerDatabase() {}
CareerDatabase::~CareerDatabase() {}

void CareerDatabase::SetPendingFixture(bool isHome, int userTeamDBID, int opponentTeamDBID,
                                       const std::string& opponentName) {
  m_pendingFixture.hasFixture = true;
  m_pendingFixture.isHome = isHome;
  m_pendingFixture.userTeamDBID = userTeamDBID;
  m_pendingFixture.opponentTeamDBID = opponentTeamDBID;
  m_pendingFixture.opponentName = opponentName;
}

bool CareerDatabase::HasPendingFixture() const {
  return m_pendingFixture.hasFixture;
}

const CareerPendingFixture& CareerDatabase::GetPendingFixture() const {
  return m_pendingFixture;
}

void CareerDatabase::ClearPendingFixture() {
  m_pendingFixture = CareerPendingFixture{};
}

bool CareerDatabase::ConsumePlayedFixture(int matchGoals0, int matchGoals1) {
  if (!m_pendingFixture.hasFixture || !m_activeSave) {
    return false;
  }

  MatchRequest request;
  request.opponentName = m_pendingFixture.opponentName;
  request.opponentTeamDBID = std::to_string(m_pendingFixture.opponentTeamDBID);
  request.userIsHome = m_pendingFixture.isHome;

  Full3DMatchEngine engine;
  const MatchResult normalized = engine.Complete(request, matchGoals0, matchGoals1);
  const int userGoals = normalized.userGoals;
  const int oppGoals = normalized.opponentGoals;

  for (auto& f : m_activeSave->season.fixtures) {
    if (!f.played &&
        ((m_pendingFixture.isHome && f.homeTeamID == m_pendingFixture.userTeamDBID &&
          f.awayTeamID == m_pendingFixture.opponentTeamDBID) ||
         (!m_pendingFixture.isHome && f.homeTeamID == m_pendingFixture.opponentTeamDBID &&
          f.awayTeamID == m_pendingFixture.userTeamDBID))) {
      f.homeGoals = m_pendingFixture.isHome ? userGoals : oppGoals;
      f.awayGoals = m_pendingFixture.isHome ? oppGoals : userGoals;
      f.played = true;
      break;
    }
  }

  m_pendingFixture = CareerPendingFixture{};

  CareerSim::Process3DMatchResult(*m_activeSave, *this, userGoals, oppGoals);
  AutoSave();
  return true;
}

bool CareerDatabase::Initialize(const std::string& saveDir) {
  m_saveDirectory = saveDir;
  return true;
}

std::string CareerDatabase::GetSlotPath(int slotIndex) const {
  return CareerSaveService::GetSlotPath(m_saveDirectory, slotIndex);
}

bool CareerDatabase::HasSaveFile() const {
  return CareerSaveService::HasSaveFile(m_saveDirectory);
}

bool CareerDatabase::HasSaveSlot(int slotIndex) const {
  return CareerSaveService::HasSaveSlot(m_saveDirectory, slotIndex);
}

bool CareerDatabase::LoadCareerSave(const std::string& saveName) {
  if (m_saveDirectory.empty())
    return false;
  if (LoadCareerSlot(0)) {
    printf("[career] Loaded default save: %s\n", saveName.c_str());
    return true;
  }
  if (LoadCareerSlot(-1)) {
    printf("[career] Loaded autosave fallback: %s\n", saveName.c_str());
    return true;
  }
  return false;
}

bool CareerDatabase::LoadCareerSlot(int slotIndex) {
  CareerSave loaded;
  std::vector<TransferBid> loadedBids;
  if (!CareerSaveService::LoadSlot(m_saveDirectory, slotIndex, loaded, loadedBids))
    return false;
  m_activeSave = std::make_unique<CareerSave>(loaded);
  m_activeBids = loadedBids;
  const std::string path = GetSlotPath(slotIndex);
  printf("[career] Loaded slot %d from %s\n", slotIndex, path.c_str());
  return true;
}

bool CareerDatabase::SaveCareerData() {
  return SaveCareerSlot(0);
}

bool CareerDatabase::SaveCareerSlot(int slotIndex) {
  if (!m_activeSave)
    return false;
  const bool success =
      CareerSaveService::SaveSlot(m_saveDirectory, slotIndex, *m_activeSave, m_activeBids);
  if (success) {
    const std::string path = GetSlotPath(slotIndex);
    printf("[career] Saved slot %d to %s\n", slotIndex, path.c_str());
  }
  return success;
}

bool CareerDatabase::DeleteCareerSlot(int slotIndex) {
  const std::string path = GetSlotPath(slotIndex);
  if (!CareerSaveService::DeleteSlot(m_saveDirectory, slotIndex))
    return false;
  printf("[career] Deleted slot %d (%s)\n", slotIndex, path.c_str());
  return true;
}

bool CareerDatabase::AutoSave() {
  if (!m_activeSave)
    return false;
  return SaveCareerSlot(-1);
}

bool CareerDatabase::GetSlotSummary(int slotIndex,
                                    CareerPersistence::CareerSaveSummary& outSummary) const {
  return CareerSaveService::GetSlotSummary(m_saveDirectory, slotIndex, outSummary);
}

bool CareerDatabase::CreateNewCareer(const std::string& careerName, const std::string& mode,
                                     const std::string& managerName) {
  m_activeSave = std::make_unique<CareerSave>(
      CareerLifecycleService::CreateInitialSave(careerName, mode, managerName));
  CareerFinance::InitializeOwnerData(*m_activeSave);
  CareerBoard::GenerateBoardObjectives(*m_activeSave);
  CareerSponsors::GenerateSponsorOffers(*m_activeSave);
  CareerTransfers::SeedFreeAgents(*m_activeSave);
  bool saved = SaveCareerData();
  AutoSave();
  return saved;
}

void CareerDatabase::AddEvent(const std::string& eventType, const std::string& description,
                              int reputationDelta, bool isMajor) {
  if (!m_activeSave)
    return;
  if (CareerEventService::AddEvent(*m_activeSave, eventType, description, reputationDelta,
                                   isMajor)) {
    // Persist on major milestones only. Routine matchday chatter must not flush
    // the save file on every simulated fixture.
    SaveCareerData();
  }
}

void CareerDatabase::RecruitFreeAgent(const std::string& playerName) {
  if (m_activeSave)
    CareerTransfers::RecruitFreeAgent(*m_activeSave, *this, playerName);
}

void CareerDatabase::ScoutYouthPlayer() {
  if (m_activeSave)
    CareerTraining::ScoutYouthPlayer(*m_activeSave, *this);
}

void CareerDatabase::PromoteYouthPlayer(const std::string& playerName) {
  if (m_activeSave)
    CareerTraining::PromoteYouthPlayer(*m_activeSave, *this, playerName);
}

void CareerDatabase::ModifyBudget(long long transferDelta, long long wageDelta) {
  if (m_activeSave)
    CareerFinance::ModifyBudget(*m_activeSave, transferDelta, wageDelta);
}

void CareerDatabase::ModifyBoardConfidence(int delta) {
  if (m_activeSave)
    CareerEventService::ModifyBoardConfidence(*m_activeSave, delta);
}

bool CareerDatabase::TrainSquad() {
  return m_activeSave && CareerTraining::TrainSquad(*m_activeSave, *this);
}

bool CareerDatabase::TrainFocus(const std::string& focusArea) {
  return m_activeSave && CareerTraining::TrainFocus(*m_activeSave, *this, focusArea);
}

bool CareerDatabase::MotivatePlayer(const std::string& playerName) {
  return m_activeSave && CareerTraining::MotivatePlayer(*m_activeSave, *this, playerName);
}

bool CareerDatabase::DrillPlayer(const std::string& playerName) {
  return m_activeSave && CareerTraining::DrillPlayer(*m_activeSave, *this, playerName);
}

void CareerDatabase::SetStrategy(const std::string& strategy) {
  if (m_activeSave)
    CareerTraining::SetStrategy(*m_activeSave, *this, strategy);
}

int CareerDatabase::GetReputation() const {
  return CareerEventService::GetReputation(m_activeSave.get());
}

std::string CareerDatabase::GetReputationStatus() const {
  return CareerPresentation::GetReputationStatus(m_activeSave.get());
}

std::string CareerDatabase::GetMoraleString(int morale) const {
  return CareerPresentation::GetMoraleString(morale);
}

std::string CareerDatabase::GetFormString(int form) const {
  return CareerPresentation::GetFormString(form);
}

std::string CareerDatabase::GetConditionArrow(int form) const {
  return CareerPresentation::GetConditionArrow(form);
}

std::string CareerDatabase::GetFormGuideString(int count) const {
  return CareerPresentation::GetFormGuideString(m_activeSave.get(), count);
}

std::vector<std::string> CareerDatabase::GetNewsHeadlines(int count) const {
  return CareerPresentation::GetNewsHeadlines(m_activeSave.get(), count);
}

std::string CareerDatabase::GetNextOpponentPreview(int week) const {
  return CareerPresentation::GetNextOpponentPreview(week);
}

int CareerDatabase::GetLegacyStat(const std::string& statName) const {
  return CareerEventService::GetLegacyStat(m_activeSave.get(), statName);
}

std::vector<CareerEvent> CareerDatabase::GetRecentEvents(int limit) const {
  return CareerEventService::GetRecentEvents(m_activeSave.get(), limit);
}

void CareerDatabase::ProcessPlayerGrowth(PlayerCareerState& player) {
  CareerSim::ProcessPlayerGrowth(player, m_activeSave.get());
}

void CareerDatabase::UpdatePlayerValue(PlayerCareerState& player) {
  CareerSim::UpdatePlayerValue(player);
}

int CareerDatabase::EstimateLeaguePosition(int wins, int draws, int losses) {
  return CareerSim::EstimateLeaguePosition(wins, draws, losses);
}

std::vector<CareerSim::CareerLeagueTableRow> CareerDatabase::GetLeagueStandings(
    const std::vector<std::pair<int, std::string>>& leagueClubs) const {
  if (!m_activeSave)
    return {};
  return CareerSim::GenerateLeagueStandings(*m_activeSave, leagueClubs);
}

std::vector<CareerSim::CareerTopScorer> CareerDatabase::GetTopScorers() const {
  if (!m_activeSave)
    return {};
  return CareerSim::GetTopScorers(*m_activeSave);
}

void CareerDatabase::AdvanceSeason() {
  if (!m_activeSave)
    return;
  CareerSim::AdvanceSeason(*m_activeSave, *this, m_activeBids, m_transferTargets);
  SaveCareerData();
  AutoSave();
}

void CareerDatabase::ReleasePlayer(const std::string& playerName) {
  if (m_activeSave)
    CareerTransfers::ReleasePlayer(*m_activeSave, *this, playerName);
}

void CareerDatabase::RecordMatchStats(const std::string& playerName, int goals, int assists) {
  if (m_activeSave)
    CareerSim::RecordMatchStats(*m_activeSave, playerName, goals, assists);
}

void CareerDatabase::PopulateTransferMarket() {
  if (m_activeSave)
    CareerTransfers::PopulateTransferMarket(m_transferTargets);
}

std::vector<TransferTarget> CareerDatabase::GetTransferTargets() const {
  return m_transferTargets;
}

TransferBid CareerDatabase::PlaceBid(const std::string& playerName, long long bidAmount,
                                     int offeredWage, int contractYears) {
  if (!m_activeSave)
    return TransferBid();
  return CareerTransfers::PlaceBid(*m_activeSave, *this, m_transferTargets, m_activeBids,
                                   playerName, bidAmount, offeredWage, contractYears);
}

void CareerDatabase::WithdrawBid(const std::string& playerName) {
  CareerTransfers::WithdrawBid(m_activeBids, playerName);
}

void CareerDatabase::ProcessPendingBids() {
  if (m_activeSave)
    CareerTransfers::ProcessPendingBids(*m_activeSave, *this, m_transferTargets, m_activeBids);
}

std::string CareerDatabase::GetBidStatusString(BidStatus status) const {
  return CareerTransfers::GetBidStatusString(status);
}

bool CareerDatabase::CompleteTransfer(const std::string& playerName) {
  if (!m_activeSave)
    return false;
  return CareerTransfers::CompleteTransfer(*m_activeSave, *this, m_transferTargets, m_activeBids,
                                           playerName);
}

void CareerDatabase::InitializeOwnerData() {
  if (m_activeSave)
    CareerFinance::InitializeOwnerData(*m_activeSave);
}

void CareerDatabase::UpgradeStadium(int upgradeIndex) {
  if (m_activeSave)
    CareerFinance::UpgradeStadium(*m_activeSave, *this, upgradeIndex);
}

void CareerDatabase::RenameStadium(const std::string& newName) {
  if (m_activeSave)
    CareerFinance::RenameStadium(*m_activeSave, newName);
}

void CareerDatabase::RepairStadium(int amount) {
  if (m_activeSave)
    CareerFinance::RepairStadium(*m_activeSave, amount);
}

void CareerDatabase::SetTicketPrice(int price) {
  if (m_activeSave)
    CareerFinance::SetTicketPrice(*m_activeSave, price);
}

void CareerDatabase::HireStaff(const StaffMember& member) {
  if (m_activeSave)
    CareerStaff::HireStaff(*m_activeSave, member);
}

void CareerDatabase::FireStaff(const std::string& staffName) {
  if (m_activeSave)
    CareerStaff::FireStaff(*m_activeSave, *this, staffName);
}

void CareerDatabase::GenerateStaffCandidates(std::vector<StaffMember>& candidates) {
  CareerStaff::GenerateStaffCandidates(candidates);
}

void CareerDatabase::GenerateSponsorOffers() {
  if (m_activeSave)
    CareerSponsors::GenerateSponsorOffers(*m_activeSave);
}

bool CareerDatabase::AcceptSponsorDeal(int dealIndex) {
  return m_activeSave && CareerSponsors::AcceptSponsorDeal(*m_activeSave, *this, dealIndex);
}

void CareerDatabase::TerminateSponsorDeal(const std::string& sponsorName) {
  if (m_activeSave)
    CareerSponsors::TerminateSponsorDeal(*m_activeSave, *this, sponsorName);
}

void CareerDatabase::ProcessSeasonFinances() {
  if (m_activeSave)
    CareerFinance::ProcessSeasonFinances(*m_activeSave);
}

long long CareerDatabase::GetSeasonProfit() const {
  return m_activeSave ? CareerFinance::GetSeasonProfit(*m_activeSave) : 0;
}

std::string CareerDatabase::GetFinancialHealthString() const {
  return m_activeSave ? CareerFinance::GetFinancialHealthString(*m_activeSave) : "Unknown";
}

void CareerDatabase::GenerateBoardObjectives() {
  if (m_activeSave)
    CareerBoard::GenerateBoardObjectives(*m_activeSave);
}

void CareerDatabase::EvaluateBoardObjectives() {
  if (m_activeSave)
    CareerBoard::EvaluateBoardObjectives(*m_activeSave, *this);
}

void CareerDatabase::InvestInFanBase(long long amount) {
  if (m_activeSave)
    CareerFinance::InvestInFanBase(*m_activeSave, amount);
}

void CareerDatabase::InvestInPrestige(long long amount) {
  if (m_activeSave)
    CareerFinance::InvestInPrestige(*m_activeSave, amount);
}

SimulatedMatch CareerDatabase::SimulateMatchResult(const std::string& opponentName,
                                                   const std::string& opponentTeamDBID,
                                                   bool isHome) {
  if (!m_activeSave)
    return SimulatedMatch{};

  MatchRequest request;
  request.opponentName = opponentName;
  request.opponentTeamDBID = opponentTeamDBID;
  request.userIsHome = isHome;

  FastMatchEngine engine(*m_activeSave);
  const MatchEngineRun run = engine.Start(request);
  return ToLegacySimulatedMatch(request, run.result);
}

void CareerDatabase::SeedRng(unsigned int seed) {
  CareerCommon::SeedRng(seed);
}

void CareerDatabase::ApplyMatchResult(int homeGoals, int awayGoals,
                                      const std::string& opponentLabel,
                                      const std::vector<std::string>& scorers) {
  if (m_activeSave)
    CareerSim::ApplyMatchResult(*m_activeSave, *this, homeGoals, awayGoals, opponentLabel, scorers);
}

void CareerDatabase::Process3DMatchResult(int homeGoals, int awayGoals) {
  if (m_activeSave)
    CareerSim::Process3DMatchResult(*m_activeSave, *this, homeGoals, awayGoals);
}

}  // namespace blunted
