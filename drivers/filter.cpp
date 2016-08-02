#include "util/dataplane.hpp"
#include "util/context.hpp"
#include "util/application.hpp"
#include "util/timer.hpp"
#include "util/system.hpp"
#include "freeflow/capture.hpp"

#include <cstring>

using namespace fp;
using namespace ff;


class Port_dummy : public Port
{
public:
  // Constructors/Destructor.
  using Port::Port;

  Port_dummy(Port::Id id, std::string const& config, std::string const& name = "")
    : Port::Port(id, name)
  { }

  ~Port_dummy() { }

  int       open() { return 0; }
  Context*  recv() { return nullptr; }
  int       send() { return 0; }
  void      send(Context*) { }
  void      close() { }
  Function  work_fn() { return nullptr; }
};



int
main(int argc, char* argv[])
{
  // Read the file containing filter instructions.
  if (argc < 2)
    throw std::runtime_error("No steve file given.");
  char const* steve_file = argv[1];

  // Load the given pcap file.
  if (argc < 3)
    throw std::runtime_error("No pcap or steve file given.");
  char const* pcap_file = argv[2];

  // Check for number of copies/iterations. Default 1.
  int iterations = 1;
  if (argc > 3)
    iterations = std::stoi(argv[3]);
  std::cout << "Iterations: " << iterations << '\n';

  // Dataplane stuff.
  Dataplane dp("dp1", steve_file);
  Port_dummy p1(1, "", "p1");

  std::cout << "Loading: " << pcap_file << '\n';
  // Open an offline stream capture.
  cap::Stream cap(cap::offline(pcap_file));
  if (cap.link_type() != cap::ethernet_link) {
    std::cerr << "error: input is not ethernet\n";
    return 1;
  }

  // Add all ports
  dp.add_port(&p1);
  dp.add_reserved_ports();
  dp.configure();
  dp.up();

  cap::Packet p;

  { // begin
    Timer t;
    while(cap.get(p)) {
      // for (int i = 0; i < iterations; ++i) {
        Byte buf[2048];
        if (p.captured_size() <= 2048)
          std::memcpy(&buf[0], p.data(), p.captured_size());
        else
          continue;
        Context cxt(buf, &dp);
        dp.process(cxt);
      // }
    }
  } // end anon


}
