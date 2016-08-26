#ifndef FP_BUFFER_HPP
#define FP_BUFFER_HPP

#include "types.hpp"
#include "context.hpp"

#include <queue>
#include <functional>
#include <mutex>
#include <vector>
#include <cstring>

namespace fp
{

class Dataplane;

// The flowpath packet buffer. Contains an ID, a packet data
// store, and the context associated with the packet. There is
// no dynamic allocation of packet contexts, and the user is
// expected to initialize the context when it is allocated.
// After a buffer has been freed, accessing the contents of
// any field in this structure results in undefined behavior.
struct Buffer
{
  // Buffer ctor.
  Buffer(int id, Dataplane* dp)
    : id_(id), data_(new Byte[2048]), cxt_(dp, {data_, 2048})
  { }

  // Accessors.
  //
  // Returns the buffer ID.
  int id() const { return id_; }
  // Returns a reference to the context associated with this packet buffer.
  Context& context() { return cxt_; }


  int id_;       // Object pool index
  Byte*   data_; // The packet data.
  Context cxt_;  // The context for the packet data.
};


// The flowpath object pool. Uses a priority_queue to manage a
// min-heap, that gives next available buffer index.
class Pool
{
public:
  using Store_type = std::vector<Buffer>;
  using Heap_type = std::priority_queue<int, std::vector<int>, std::greater<int>>;
  using Mutex_type = std::mutex;

  Pool(Dataplane*);

  Pool(int, Dataplane*);

  ~Pool();

  // Buffer accessor.
  inline Buffer& operator[](int);

  // Returns the next free index from the min-heap.
  inline Buffer& alloc();

  // Returns copy of a context into the next free buffer in the min-heap.
  inline Buffer& copy(Context&);

  // Places the given index back into the min-heap.
  inline void dealloc(int);

private:
  // The buffer data store.
  Store_type data_;
  // The free-list, a min-heap.
  Heap_type  heap_;
  // Mutex for concurrency operations.
  Mutex_type mutex_;
};



// Returns a reference to the buffer at the given index.
inline Buffer&
Pool::operator[](int idx)
{
  return data_[idx];
}


// Returns a reference to the next free buffer using the min-heap.
inline Buffer&
Pool::alloc()
{
  //while (heap_.empty()) { }
  // Lock the heap.
  mutex_.lock();

  // Get the next available index.
  int id(heap_.top());

  // Remove index from the heap.
  heap_.pop();

  // Unlock the heap.
  mutex_.unlock();

  // Return a reference to the buffer at the index.
  return data_[id];
}


inline Buffer&
Pool::copy(Context& cxt)
{
  //while (heap_.empty()) { }
  // Lock the heap.
  mutex_.lock();

  // Get the next available index.
  int id(heap_.top());

  // Remove index from the heap.
  heap_.pop();

  // Unlock the heap.
  mutex_.unlock();

  // Return a reference to the buffer at the index.
  Buffer& buf = data_[id];
  buf.context() = cxt; // Trivial copy on every field.
  buf.context().packet().buf_ = buf.data_; // Reset the data pointer to the new buffer.
  // Deep copy.
  std::copy(cxt.packet().data(), cxt.packet().data() + cxt.packet().size(),
            buf.context().packet().data());

  return buf;
}



// Places the given index back into the min-heap.
inline void
Pool::dealloc(int id)
{
  // Lock the heap.
  mutex_.lock();

  // Return the index to the heap.
  heap_.push(id);

  // Unlock the heap.
  mutex_.unlock();
}



// The flowpath buffer pool singleton namespace. Used to
// statically initialize a new instance of a buffer pool.
namespace Buffer_pool
{


Pool& get_pool(Dataplane* dp);

}


} // end namespace fp

#endif
