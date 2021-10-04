#include "../Portaudio.hpp"
#include "../Config.hpp"

int main() {
  using namespace fenestra;

  Config cfg;
  Portaudio portaudio(cfg);

  auto devices = portaudio.list_devices();

  for (auto const & [ host_api_name, host_api_devices ] : devices) {
    std::cout << host_api_name << ": " << std::endl;
    for (auto const & device_name : host_api_devices) {
      std::cout << "  " << device_name << std::endl;
    }
  }
}
