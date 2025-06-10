#ifndef LIFECYCLE_MW_SERVICE_HPP_
#define LIFECYCLE_MW_SERVICE_HPP_

#include <string>
#include <vector>

namespace lifecycle_manager {

class MWService {
  public:
    MWService();
    virtual ~MWService();

    std::string GetName();
    std::string GetPath();
    std::vector<std::string> GetArgs();

  private:
    std::string mw_service_name_;
    std::string mw_service_path_;
    std::vector<std::string> mw_run_args_;

};

} // namespace lifecycle_manager

#endif // LIFECYCLE_MW_SERVICE_HPP_
