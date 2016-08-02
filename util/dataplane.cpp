#include <algorithm>

#include "dataplane.hpp"
#include "context.hpp"
#include "timer.hpp"
#include "system.hpp"

#include <iostream>
#include <cassert>

namespace fp {

// Data plane ctor.
Dataplane::Dataplane(std::string const& name, std::string const& app_name)
  : name_(name), app_(Application(app_name.c_str())), ports_(),  portmap_()
{
}

Dataplane::~Dataplane()
{
}


// Adds the port to the local list.
void
Dataplane::add_port(Port* p)
{
  ports_.push_back(p);
  portmap_.emplace(p->id(), p);
}


// Removes the port from the local list if it exists.
void
Dataplane::remove_port(Port* p)
{
	auto iter = std::find(ports_.begin(), ports_.end(), p);
  portmap_.erase(p->id());
  ports_.erase(iter);
  delete p;
}


// Add an explicit drop port to the dataplane.
void
Dataplane::add_drop_port()
{
  drop_ = new Port_reserved(0xfffffff0, ":8673", "drop");
  ports_.push_back(drop_);
  portmap_.emplace(drop_->id(), drop_);
}


// Add an explicit all port to the dataplane.
void
Dataplane::add_all_port()
{
  all_ = new Port_reserved(0xffffffef, ":8674", "all");
  ports_.push_back(all_);
  portmap_.emplace(all_->id(), all_);
}


// Add an explicit all port to the dataplane.
void
Dataplane::add_flood_port()
{
  flood_ = new Port_reserved(0xffffffee, ":8675", "flood");
  ports_.push_back(flood_);
  portmap_.emplace(flood_->id(), flood_);
}


// Add an explicit reflow port to the dataplane.
void
Dataplane::add_reflow_port()
{
  reflow_ = new Port_reserved(0xffffffed, ":8676", "reflow");
  ports_.push_back(reflow_);
  portmap_.emplace(reflow_->id(), reflow_);
}


// Add all reserved ports.
void
Dataplane::add_reserved_ports()
{
  add_drop_port();
  add_all_port();
  add_flood_port();
  add_reflow_port();
}


// Starts the data plane packet processors. If the application has configured
// the data plane, it will install the application in the thread pool and start
// it. Otherwise it reports errors.
void
Dataplane::up()
{
  app_.start(*this);
}



// For manually passing in packets to the data plane.
void
Dataplane::process(Context& cxt)
{
  try
  {
    app_.process(cxt);
  }
  catch (std::exception& e)
  {
    // Drop the packet.
    // Do nothing.
  }
}


// Stops the data plane packet processors, if they are running.
void
Dataplane::down()
{
 //  if (app_->state() == Application::State::RUNNING) {
 //   thread_pool.stop();
 //   thread_pool.uninstall();
 // }
 // else
 //   throw std::string("Data plane is not running.");
}


// Configures the data plane based on the application.
void
Dataplane::configure()
{
  app_.load(*this);
}


// Gets the data plane name.
std::string
Dataplane::name() const
{
  return name_;
}


// Gets the data planes application.
Application const&
Dataplane::app() const
{
  return app_;
}


// Gets the data planes tables.
std::vector<Table*>
Dataplane::tables() const
{
  return tables_;
}


// Gets the table at the given index.
Table*
Dataplane::table(int idx)
{
  return tables_.at(idx);
}


} // end namespace fp
