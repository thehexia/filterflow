#include "context.hpp"
#include "dataplane.hpp"



namespace fp
{

void
Context::write_metadata(uint64_t meta)
{
  metadata_.data = meta;
}


Metadata const&
Context::read_metadata()
{
  return this->metadata_;
}


Port*
Context::output_port() const
{
  return ctrl_.out_port == 0 ? dp_->get_drop_port() : dp_->get_port(ctrl_.out_port);
}


Port*
Context::input_port() const
{
  return dp_->get_port(input_.in_port);
}


Port*
Context::input_physical_port() const
{
  return dp_->get_port(input_.in_phy_port);
}


// -------------------------------------------------------------------------- //
// Evaluation of actions

namespace
{

inline void
apply(Context& cxt, Set_action a)
{

}


inline void
apply(Context& cxt, Copy_action a)
{
}


inline void
apply(Context& cxt, Output_action a)
{
  // cxt.out_port = a.port;
}


inline void
apply(Context& cxt, Queue_action a)
{
  // TODO: Implement queues.
}


inline void
apply(Context& cxt, Group_action a)
{
  // TODO: Implement group actions.
}


} // namespace


void
Context::apply_action(Action a)
{
  switch (a.type) {
    case Action::SET: return apply(*this, a.value.set);
    case Action::COPY: return apply(*this, a.value.copy);
    case Action::OUTPUT: return apply(*this, a.value.output);
    case Action::QUEUE: return apply(*this, a.value.queue);
    case Action::GROUP: return apply(*this, a.value.group);
  }
}


} // namespace fp
