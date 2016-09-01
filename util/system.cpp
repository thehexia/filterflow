#include <exception>
#include <unordered_map>
#include <cstdarg>


#include "system.hpp"
#include "application.hpp"
#include "endian.hpp"
#include "context.hpp"
#include "buffer.hpp"
#include "dataplane.hpp"


//////////////////////////////////////////////////////////////////////////
//                    External Runtime System Calls                     //
//////////////////////////////////////////////////////////////////////////
//
// These are the set of system calls that an application can expect
// to be able to call at runtime.


extern "C"
{


// -------------------------------------------------------------------------- //
// Control instructions


// Send the packet through the drop port.
void
fp_drop(fp::Context* cxt)
{
  // std::cout << "drop\n";
  fp::Port* drop = cxt->dataplane()->get_drop_port();
  assert(drop);
  cxt->set_output_port(drop->id());
}


void
fp_flood(fp::Context* cxt)
{
  // FIXME: Make a flood port.
  fp::Port* flood = cxt->dataplane()->get_flood_port();
  assert(flood);
  cxt->set_output_port(flood->id());;
}


// Outputs the contexts packet on the port with the matching name.
void
fp_output_port(fp::Context* cxt, fp::Port::Id id)
{
  // Allocate a copy.
  fp::Buffer& buf = cxt->dataplane()->buf_pool()->copy(*cxt);
  cxt->dataplane()->get_port(id)->send(buf.context());
  cxt->dataplane()->buf_pool()->dealloc(buf.id());
}


// Apply the given action to the context.
void
fp_apply(fp::Context* cxt, fp::Action a)
{
  cxt->apply_action(a);
}


// Write the given action to the context's action list.
void
fp_write(fp::Context* cxt, fp::Action a)
{
  cxt->write_action(a);
}


// Dispatches the given context to the given table, if it exists.
// Accepts a variadic list of fields needed to construct a key to
// match against the table.
void
fp_goto_table(fp::Context* cxt, fp::Table* tbl, int n, ...)
{
  va_list args;
  va_start(args, n);
  fp::Key key = fp_gather(cxt, tbl->key_size(), n, args);
  va_end(args);

  fp::Flow flow = tbl->search(key);
  // execute the flow function
  flow.instr_(&flow, tbl, cxt);
}


// -------------------------------------------------------------------------- //
// Port and table operations

// Returns the port matching the given id or error otherwise.
fp::Port::Id
fp_get_port_by_id(fp::Dataplane* dp, unsigned int id)
{
  // std::cout << "GETTING PORT\n";
  fp::Port* p = dp->get_port(id);
  // std::cout << "FOUND PORT\n";
  assert(p);
  return id;
}

// Returns whether or not the port is up or down
bool
fp_port_id_is_up(fp::Dataplane* dp, fp::Port::Id id)
{
  assert(dp);
  fp::Port* p = dp->get_port(id);
  return !p->is_down();
}

// Returns whether or not the given id exists.
bool
fp_port_id_is_down(fp::Dataplane* dp, fp::Port::Id id)
{
  assert(dp);
  fp::Port* p = dp->get_port(id);
  return p->is_down();
}

// Returns the port id to the all port;
fp::Port::Id
fp_get_all_port(fp::Dataplane* dp)
{
  assert(dp);
  fp::Port* p = dp->get_all_port();
  return p->id();
}


fp::Port::Id
fp_get_reflow_port(fp::Dataplane* dp)
{
  assert(dp);
  fp::Port* p = dp->get_reflow_port();
  return p->id();
}


fp::Port::Id
fp_get_flood_port(fp::Dataplane* dp)
{
  assert(dp);
  fp::Port* p = dp->get_flood_port();
  return p->id();
}


// Copies the values within 'n' fields into a byte buffer
// and constructs a key from it.
//
// 255 is an alias for ingress port, 256 is an alias for physical ingress port.
fp::Key
fp_gather(fp::Context* cxt, int key_width, int n, va_list args)
{
  // FIXME: We're using a fixed size key of 128 bytes right now
  // apparently. This should probably be dynamic.
  fp::Byte buf[fp::key_size];
  // Iterate through the fields given in args and copy their
  // values into a byte buffer.
  int i = 0;
  int j = 0;
  int in_port;
  int in_phy_port;
  while (i < n) {
    int f = va_arg(args, int);
    fp::Binding b;
    fp::Byte* p = nullptr;

    // Check for "Special fields"
    switch (f) {
      // Looking for "in_port"
      case 255:
        in_port = cxt->input_port_id();
        p = reinterpret_cast<fp::Byte*>(&in_port);
        // Copy the field into the buffer.
        std::copy(p, p + sizeof(in_port), &buf[j]);
        j += sizeof(in_port);
        break;

      // Looking for "in_phys_port"
      case 256:
        in_phy_port = cxt->input_physical_port_id();
        p = reinterpret_cast<fp::Byte*>(&in_phy_port);
        // Copy the field into the buffer.
        std::copy(p, p + sizeof(in_phy_port), &buf[j]);
        j += sizeof(in_phy_port);
        break;

      // Regular fields
      default:
        // Lookup the field in the context.
        b = cxt->get_field_binding(f);
        p = cxt->get_field(b.offset);
        // Copy the field into the buffer.
        std::copy(p, p + b.length, &buf[j]);
        // Then reverse the field in place.
        fp::network_to_native_order(&buf[j], b.length);
        j += b.length;
        break;
    }
    ++i;
  }

  return fp::Key(buf, key_width);
}


// Creates a new table in the given data plane with the given size,
// key width, and table type.
fp::Table*
fp_create_table(fp::Dataplane* dp, int id, int key_width, int size, fp::Table::Type type)
{
  fp::Table* tbl = nullptr;
  std::cout << "Create table\n";

  switch (type)
  {
    case fp::Table::Type::EXACT:
    // Make a new hash table.
    tbl = new fp::Hash_table(id, size, key_width);
    assert(tbl);
    dp->tables_.push_back(tbl);
    break;
    case fp::Table::Type::PREFIX:
    // Make a new prefix match table.
    break;
    case fp::Table::Type::WILDCARD:
    // Make a new wildcard match table.
    break;
    default:
    throw std::string("Unknown table type given");
  }

  std::cout << "Returning table\n";
  return tbl;
}


// Creates a new flow rule from the given key and function pointer
// and adds it to the given table.
//
// FIXME: Currently ignoring timeout.
void
fp_add_init_flow(fp::Table* tbl, void* fn, void* key, unsigned int timeout, unsigned int egress)
{
  // get the length of the table's expected key
  int key_size = tbl->key_size();
  // cast the key to Byte*
  fp::Byte* buf = reinterpret_cast<fp::Byte*>(key);
  // construct a key object
  fp::Key k(buf, key_size);
  // cast the flow into a flow instruction
  fp::Flow_instructions instr = reinterpret_cast<fp::Flow_instructions>(fn);
  fp::Flow flow(0, fp::Flow_counters(), instr, fp::Flow_timeouts(), 0, 0, egress);

  tbl->add(k, flow);
}


// FIXME: Ignoring timeouts.
void
fp_add_new_flow(fp::Table* tbl, void* fn, void* key, unsigned int timeout, unsigned int egress)
{
  int key_size = tbl->key_size();
  // cast the key to Byte*
  fp::Byte* buf = reinterpret_cast<fp::Byte*>(key);
  // construct a key object
  fp::Key k(buf, key_size);
  // cast the flow into a flow instruction
  fp::Flow_instructions instr = reinterpret_cast<fp::Flow_instructions>(fn);
  fp::Flow flow(0, fp::Flow_counters(), instr, fp::Flow_timeouts(), 0, 0, egress);

  tbl->add(k, flow);
}


fp::Port::Id
fp_get_flow_egress(fp::Flow* f)
{
  assert(f);
  assert(f->egress_ != 0);
  return f->egress_;
}


// Adds the miss case for the table.
//
// FIXME: Ignoring timeout value.
void
fp_add_miss(fp::Table* tbl, void* fn, unsigned int timeout, unsigned int egress)
{
  // cast the flow into a flow instruction
  fp::Flow_instructions instr = reinterpret_cast<fp::Flow_instructions>(fn);
  fp::Flow flow(0, fp::Flow_counters(), instr, fp::Flow_timeouts(), 0, 0, egress);
  tbl->insert_miss(flow);
}


// Removes the given key from the given table, if it exists.
void
fp_del_flow(fp::Table* tbl, void* key)
{
  // get the length of the table's expected key
  int key_size = tbl->key_size();
  // cast the key to Byte*
  fp::Byte* buf = reinterpret_cast<fp::Byte*>(key);
  // construct a key object
  fp::Key k(buf, key_size);
  // delete the key
  tbl->rmv(k);
}

// Removes the miss case from the given table and replaces
// it with the default.
void
fp_del_miss(fp::Table* tbl)
{
  tbl->rmv_miss();
}


// Raise an event.
// TODO: Make this asynchronous on another thread.
void
fp_raise_event(fp::Context* cxt, void* handler)
{
  // Cast the handler back to its appropriate function type
  // of void (*)(Context*)
  void (*event)(fp::Context*);
  event = (void (*)(fp::Context*)) (handler);
  // Invoke the event.
  // FIXME: This should produce a copy of the context and process it
  // seperately.
  event(cxt);
}


} // extern "C"
