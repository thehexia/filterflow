#include "port.hpp"
#include "context.hpp"

#include <climits>
#include <netinet/in.h>
#include <cassert>
#include <iostream>
#include <cstring>

namespace fp
{

// Port definitions.
//


// Port constructor that sets ID.
Port::Port(Port::Id id, std::string const& name)
  : id_(id), name_(name), stats_(), config_(), state_()
{ }


// Port dtor.
Port::~Port()
{ }


// Comparators.
//
bool
operator==(Port const& lhs, Port const& rhs)
{
  return lhs.id() == rhs.id();
}


bool
operator==(Port* p, std::string const& name)
{
  return p->name() == name;
}


bool
operator!=(Port const& lhs, Port const& rhs)
{
  return !(lhs.id() == rhs.id());
}


//----------------------------------------------------------------------------//
// Pcap port
//----------------------------------------------------------------------------//


Port_pcap::Port_pcap(Port::Id id, char const* pfile, Mode mode, std::string const& name = "")
  : Port::Port(id, name), pfile_(pfile), mode_(mode)
{
  switch (mode) {
    case Mode::READ_OFFLINE:
      stream_.read_ = new Stream(ff::cap::offline(pfile));
      break;

    case Mode::WRITE_OFFLINE:
      stream_.dump_ = new Dump_stream(pfile);
      break;

    case Mode::READ_LIVE:
    case Mode::WRITE_LIVE:
      break;
  }
}


Port_pcap::~Port_pcap()
{
  switch (this->mode()) {
    case Mode::READ_OFFLINE:
      delete stream_.read_;
      break;

    case Mode::WRITE_OFFLINE:
      delete stream_.dump_;
      break;

    case Mode::READ_LIVE:
    case Mode::WRITE_LIVE:
      break;
  }
}


bool
Port_pcap::send(Context& cxt)
{
  switch (this->mode()) {
    case Mode::WRITE_OFFLINE:
      return send_offline(cxt);
    case Mode::WRITE_LIVE:
      return false;

    default: return false;
  }

  // Otherwise this isn't a sending port.
  return false;
}


bool
Port_pcap::recv(Context& cxt)
{
  switch (this->mode()) {
    case Mode::READ_OFFLINE:
      return recv_offline(cxt);
    case Mode::READ_LIVE:
      return false;

    default: return false;
  }

  // Otherwise this isn't a receiving port.
  return false;
}


bool
Port_pcap::send_offline(Context& cxt)
{
  assert(this->mode() == Mode::WRITE_OFFLINE);

  assert(cxt.packet().data());
  // TODO: Handle the pcap header better.
  pcap_pkthdr hdr {{0,0}, cxt.size(), cxt.size()};
  ff::cap::Packet p;

  p.hdr = &hdr;
  p.buf = cxt.packet().data();
  stream_.dump_->dump(p);

  return true;
}


bool
Port_pcap::recv_offline(Context& cxt)
{
  assert(this->mode() == Mode::READ_OFFLINE);
  static short recv_mtu = 2048; // Current max buffer size on contexts.

  ff::cap::Packet p;
  if (stream_.read_->get(p)) {
    if (p.captured_size() > recv_mtu)
      throw std::runtime_error("Packet over MTU");

    cxt.set_input(this, this, 0);
    cxt.packet().size_ = p.captured_size();
    std::memcpy(&cxt.packet().data()[0], p.data(), p.captured_size());
    return true;
  }

  return false;
}


// void
// Port_pcap::send(Context* cxt)
// {
//   assert(cxt->packet().data());
//   pcap_pkthdr hdr {{0,0}, cxt->size(), cxt->size()};
//   cap::Packet p;
//
//   p.hdr = &hdr;
//   p.buf = cxt->packet().data();
//   dump_.dump(p);
// }
//
//
// void
// Port_pcap::send(cap::Packet& p)
// {
//   dump_.dump(p);
// }
//


} // end namespace FP
