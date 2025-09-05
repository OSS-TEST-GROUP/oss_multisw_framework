#include <thread>

#include "otac_main/OtacMain.hpp"

using namespace ota_client::agl;

int main(int argc, char* argv[]) {

    OtacMain otac_main_;
    if (otac_main_.Initialize()) {
        otac_main_.Start();
    } else {
        otac_main_.Stop();
    }

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}