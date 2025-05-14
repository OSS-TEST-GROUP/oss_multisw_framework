#include "DiagnosticManager.hpp"

namespace diagnostic_manager {

DiagnosticManager::DiagnosticManager() {}
DiagnosticManager::~DiagnosticManager() {}

bool DiagnosticManager::Initialize() {
    return true;
}

bool DiagnosticManager::Start() {
    return true;
}
void DiagnosticManager::Stop() {}

} // namespace diagnostic_manager
