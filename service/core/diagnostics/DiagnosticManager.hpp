#ifndef DIAGNOSTICS_DIAGNOSTIC_MANAGER_HPP_
#define DIAGNOSTICS_DIAGNOSTIC_MANAGER_HPP_

namespace diagnostic_manager {

class DiagnosticManager {
  public:
    DiagnosticManager();
    virtual ~DiagnosticManager();

    bool Initialize();

    bool Start();
    void Stop();

  private:
    
};

} // namespace diagnostic_manager

#endif
