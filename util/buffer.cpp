#include "buffer.hpp"


namespace fp
{

// Buffer pool default ctor.
Pool::Pool(Dataplane* dp)
  : Pool(4096, dp)
{ }


// Buffer pool sized ctor. Intializes the free-list (min-heap)
// and the pool of buffers.
Pool::Pool(int size, Dataplane* dp)
  : data_(), heap_(), mutex_()
{
  for (int i = 0; i < size; i++) {
    heap_.push(i);
    data_.push_back(Buffer(i, dp));
  }
}


// Buffer pool dtor.
Pool::~Pool()
{ }

namespace Buffer_pool
{

Pool&
get_pool(Dataplane* dp)
{
  static Pool p(1024 * 256 + 1024, dp);
  return p;
}


} // namespace Buffer_pol

} // namespace fp
