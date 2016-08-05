#include "util/dataplane.hpp"
#include "util/context.hpp"
#include "util/application.hpp"
#include "util/timer.hpp"
#include "util/system.hpp"
#include "freeflow/capture.hpp"

#include <cstring>

using namespace fp;
using namespace ff;


class Port_dump : public Port
{
public:
  // Constructors/Destructor.
  using Port::Port;

  Port_dump(Port::Id id, char const* dfile, std::string const& name = "")
    : Port::Port(id, name), dump_(dfile)
  { }

  ~Port_dump() { }

  int       open() { return 0; }
  Context*  recv() { return nullptr; }
  int       send() { return 0; }
  void      send(Context*);
  void      send(cap::Packet&);
  void      close() { }
  Function  work_fn() { return nullptr; }

private:
  cap::Dump_stream dump_;
};


void
Port_dump::send(Context* cxt)
{ }


void
Port_dump::send(cap::Packet& p)
{
  dump_.dump(p);
}


int
main(int argc, char* argv[])
{
  // Read the file containing filter instructions.
  if (argc < 2)
    throw std::runtime_error("Usage: driver <steve-program> <pcap-file> <output-file> [ <iterations> ]");
  char* steve_file = argv[1];

  // Load the given pcap file.
  if (argc < 3)
    throw std::runtime_error("Usage: driver <steve-program> <pcap-file> <output-file> [ <iterations> ]");
  char* pcap_file = argv[2];

  // Get the dump output file.
  if (argc < 4)
    throw std::runtime_error("Usage: driver <steve-program> <pcap-file> <output-file> [ <iterations> ]");
  char* dump_file = argv[3];

  // Check for number of copies/iterations. Default 1.
  int iterations = 1;
  if (argc > 4)
    iterations = std::stoi(argv[4]);
  std::cout << "Iterations: " << iterations << '\n';

  // Dataplane stuff.
  Dataplane dp("dp1", steve_file);
  Port_dump p1(1, dump_file, "p1");

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
      for (int i = 0; i < iterations; ++i) {
        Byte buf[2048];
        if (p.captured_size() <= 2048)
          std::memcpy(&buf[0], p.data(), p.captured_size());
        else
          continue;
        Context cxt(buf, &dp, p1.id(), p1.id(), 0);
        dp.process(cxt);

        // Send packet. We'll treat this as a sort of echo server.
        // The ingress port is set to the dump port we created earliar.
        // If the application wants to let the packet through, it will set the
        // egress port = ingress port. Otherwise it will set to drop port.
        cxt.output_port()->send(p);
      }
    }
  } // end anon


}
