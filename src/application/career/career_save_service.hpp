#ifndef CAREER_SAVE_SERVICE_HPP
#define CAREER_SAVE_SERVICE_HPP

#include <filesystem>
#include <string>
#include <vector>

#include "data/careerdata.hpp"
#include "persistence/career/career_persistence.hpp"

namespace blunted {
namespace CareerSaveService {

inline std::string GetSlotPath(const std::string& saveDirectory, int slotIndex) {
  const std::string dir = saveDirectory.empty() ? "." : saveDirectory;
  if (slotIndex == -1)
    return dir + "/career_autosave.save";
  if (slotIndex == 0)
    return dir + "/career.save";
  return dir + "/career_slot_" + std::to_string(slotIndex) + ".save";
}

inline bool HasSaveSlot(const std::string& saveDirectory, int slotIndex) {
  if (saveDirectory.empty())
    return false;

  CareerPersistence::CareerSaveSummary summary;
  return CareerPersistence::ReadSummary(GetSlotPath(saveDirectory, slotIndex), summary) &&
         summary.isValid;
}

inline bool HasSaveFile(const std::string& saveDirectory) {
  if (saveDirectory.empty())
    return false;
  return HasSaveSlot(saveDirectory, 0) || HasSaveSlot(saveDirectory, -1);
}

inline bool LoadSlot(const std::string& saveDirectory, int slotIndex, CareerSave& save,
                     std::vector<TransferBid>& bids) {
  if (saveDirectory.empty())
    return false;
  return CareerPersistence::Load(save, bids, GetSlotPath(saveDirectory, slotIndex));
}

inline bool SaveSlot(const std::string& saveDirectory, int slotIndex, const CareerSave& save,
                     const std::vector<TransferBid>& bids) {
  if (saveDirectory.empty())
    return false;
  return CareerPersistence::Save(save, bids, GetSlotPath(saveDirectory, slotIndex));
}

inline bool DeleteSlot(const std::string& saveDirectory, int slotIndex) {
  if (saveDirectory.empty() || !HasSaveSlot(saveDirectory, slotIndex))
    return false;

  const std::string path = GetSlotPath(saveDirectory, slotIndex);
  std::error_code ec;
  std::filesystem::remove(path, ec);
  std::filesystem::remove(path + ".bak", ec);
  std::filesystem::remove(path + ".tmp", ec);
  return true;
}

inline bool GetSlotSummary(const std::string& saveDirectory, int slotIndex,
                           CareerPersistence::CareerSaveSummary& outSummary) {
  if (saveDirectory.empty())
    return false;
  return CareerPersistence::ReadSummary(GetSlotPath(saveDirectory, slotIndex), outSummary);
}

}  // namespace CareerSaveService
}  // namespace blunted

#endif  // CAREER_SAVE_SERVICE_HPP
