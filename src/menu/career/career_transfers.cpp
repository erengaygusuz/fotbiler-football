// Transitional compatibility translation unit for the legacy menu build list.
// Canonical transfer rules live under src/core/career; SQLite access remains
// behind the repository adapter under src/persistence/career. Legacy no-repo
// entry points and localized labels are header-only at the menu edge.
#include "core/career/career_transfers.cpp"
#include "persistence/career/sqlite_transfer_market_repository.cpp"
