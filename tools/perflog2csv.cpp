#include "PerflogReader.hpp"

#include <iostream>

int main(int argc, char * argv[]) {
  // TODO: add --tail flag
  if (argc < 1) {
    std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
    return 1;
  }

  PerflogReader reader(argv[1]);

  std::uint64_t last_time = 0;

  for (;;) {
    reader.poll();

    if (reader.time() > last_time) {
      if (last_time == 0) {
        std::string header_line;
        for (auto const & queue : reader.queues()) {
          if (header_line.length() > 0) {
            header_line += ",";
          }
          // TODO: Replace leading spaces
          header_line += queue.name();
        }
        std::cout << header_line << std::endl;

        // TODO: Write out the full contents of the queue
      }

      std::string line;
      for (auto const & queue : reader.queues()) {
        if (line.length() > 0) {
          line += ",";
        }
        line += std::to_string(queue[queue.size() - 1]);
      }
      std::cout << line << std::endl;

      last_time = reader.time();
    }
  }
}
