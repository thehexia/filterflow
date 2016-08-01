#ifndef FP_SYSTEM_HPP
#define FP_SYSTEM_HPP

#include "port.hpp"
#include "table.hpp"
#include "action.hpp"
// #include "thread.hpp"

extern "C"
{

// Apply actions.
void           fp_drop(fp::Context*);
void           fp_flood(fp::Context*);
void           fp_goto_table(fp::Context*, fp::Table*, int, ...);
void           fp_output_port(fp::Context*, fp::Port::Id);

// System queries.
fp::Dataplane* fp_get_dataplane(std::string const&);
// fp::Port::Id   fp_get_port_by_name(char const*);
fp::Key        fp_gather(fp::Context*, int, int, va_list);
fp::Port::Id   fp_get_flow_egress(fp::Flow*);
fp::Port::Id   fp_get_port_by_id(fp::Dataplane*, unsigned int);
bool           fp_port_id_is_up(fp::Dataplane*, fp::Port::Id);
bool           fp_port_id_is_down(fp::Dataplane*, fp::Port::Id);
fp::Port::Id   fp_get_all_port(fp::Dataplane*);
fp::Port::Id   fp_get_reflow_port(fp::Dataplane*);
fp::Port::Id   fp_get_flood_port(fp::Dataplane*);

// Flow tables.
fp::Table*     fp_create_table(fp::Dataplane*, int, int, int, fp::Table::Type);
void           fp_delete_table(fp::Dataplane*, fp::Table*);
void           fp_add_init_flow(fp::Table*, void*, void*, unsigned int, unsigned int);
void           fp_add_new_flow(fp::Table*, void*, void*, unsigned int, unsigned int);
void           fp_add_miss(fp::Table*, void*, unsigned int, unsigned int);
void           fp_del_flow(fp::Table*, void*);
void           fp_del_miss(fp::Table*);

void           fp_raise_event(fp::Context*, void*);

} // extern "C"


namespace fp
{

struct Dataplane;

// Port management functions.
//
Port*             create_port(Port::Type, std::string const&, std::string const&);
void 	            delete_port(Port::Id);

// Data plane management functions.
//
Dataplane*             create_dataplane(std::string const&, std::string const&);
void			             delete_dataplane(std::string const&);

// Application management functions.
//
void load_application(std::string const&);
void unload_application(std::string const&);

} // end namespace fp

#endif
