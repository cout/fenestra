#include "PerflogViewer.hpp"
#include "PerflogReader.hpp"

#include <iostream>

int main(int argc, char * argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
    return 1;
  }

  PerflogReader reader(argv[1]);
  PerflogViewer app(reader, 752, 900);

  while (!app.done()) {
    app.poll();
    app.redraw();
    app.refresh();
  }
}
