#ifndef I_BASE_SERVICE_HPP_
#define I_BASE_SERVICE_HPP_

namespace ota_client::agl {

    class IBaseService {
        public:
            virtual ~IBaseService() = default;
            virtual bool Initialize() = 0;
            virtual bool Start() = 0;
            virtual void Stop() = 0;
    };
};

#endif