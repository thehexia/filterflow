#ifndef FP_PORT_HPP
#define FP_PORT_HPP

#include <string>
#include <netinet/in.h>

#include "freeflow/capture.hpp"
#include "packet.hpp"


// The Flowpath namespace.
namespace fp
{

struct Context;

// The base class for any port object. Contains methods for receiving,
// sending, and dropping packets, as well as the ability to close the
// port and modify its configuration.
class Port
{
public:
  enum Type { udp, tcp };

  // Port specific types.
  using Id = unsigned int;
  using Address = unsigned char const*;
  using Label = std::string;
  using Descriptor = int;

  // A port's configuration. These states can be set to determine
  // the behavior of the port.
  struct Configuration
  {
    bool down      : 1; // Port is administratively down.
    bool no_recv   : 1; // Drop all packets received by this port.
    bool no_fwd    : 1; // Drop all packets sent from this port.
    bool no_pkt_in : 1; // Do not send packet-in messages for port.
  };

  // A port's current state. These describe the observable state
  // of the device and cannot be modified.
  struct State
  {
    bool link_down : 1;
    bool blocked   : 1;
    bool live      : 1;
  };

  // Port statistics.
  struct Statistics
  {
    uint64_t packets_rx;
    uint64_t packets_tx;
    uint64_t bytes_rx;
    uint64_t bytes_tx;
  };

  // Ctor/Dtor.
  Port(Id, std::string const& = "");
  virtual ~Port();

  // The set of necessary port related functions that any port-type
  // must define.
  virtual bool open() = 0;
  virtual bool close() = 0;
  virtual bool send(Context&) = 0;
  virtual bool recv(Context&) = 0;

  // Set the ports state to 'up' or 'down'.
  void up();
  void down();

  bool is_link_down() const  { return state_.link_down; }
  bool is_admin_down() const { return config_.down; }

  // Returns true if the port is either administratively or
  // physically down.
  bool is_down() const { return is_admin_down() || is_link_down(); }

  // Returns true if the port is, in some wy, active. This
  // means that it can potentially send or receive packets.
  bool is_up() const   { return !is_down(); }


  // Accessors.
  Id          id() const    { return id_; }
  Label       name() const  { return name_; }
  Statistics  stats() const { return stats_; }

protected:
  Id              id_;        // The internal port ID.
  Address         addr_;      // The hardware address for the port.
  Label           name_;      // The name of the port.
  Statistics      stats_;     // Statistical information about the port.
  Configuration   config_;    // The current port configuration.
  State           state_;     // The runtime state of the port.
};


// Changes the port configuration to 'up'.
inline void
Port::up()
{
  config_.down = false;

  // FIXME: Actually reset stats?
  stats_ = Statistics();
}


// Change the port to a 'down' configuration.
inline void
Port::down()
{
  config_.down = true;
}


bool operator==(Port const&, Port const&);
bool operator==(Port*, std::string const&);
bool operator!=(Port const&, Port const&);


// Dummy class used to represent reserved ports....
class Port_reserved : public Port
{
public:
  // Constructors/Destructor.
  using Port::Port;

  Port_reserved(Port::Id id, std::string const& config, std::string const& name = "")
    : Port::Port(id, name)
  { }

  ~Port_reserved() { }

  virtual bool open() { return true; };
  virtual bool close() { return true; };
  // TODO: Unimplemented.
  virtual bool send(Context&) { return true; }
  virtual bool recv(Context&) { return true; }
};


class Port_pcap : public Port
{
public:
  // Constructors/Destructor.
  using Port::Port;
  using Stream = ff::cap::Stream;
  using Dump_stream = ff::cap::Dump_stream;

  // Pcap Mode.
  // TODO: Only read/write offline currently implemented.
  enum Mode {
    READ_LIVE,
    READ_OFFLINE,
    WRITE_OFFLINE,
    WRITE_LIVE
  };

  Port_pcap(Port::Id, char const*, Mode, std::string const&);
  ~Port_pcap();

  virtual bool open() { return true; };
  virtual bool close() { return true; };
  // TODO: Unimplemented.
  virtual bool send(Context&);
  virtual bool recv(Context&);

  Mode mode() const { return mode_; }

private:
  bool send_offline(Context&);
  bool recv_offline(Context&);

  char const* pfile_;
  Mode mode_;
  union
  {
    Stream* read_; // Fore reading live or offline.
    Dump_stream* dump_; // For writing offline.
  } stream_;
};


} // end namespace FP

#endif
