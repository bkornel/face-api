#pragma once

namespace fw
{
  // Erases every element the predicate accepts. Unlike std::remove_if this works on the
  // associative containers too, which cannot be reordered.
  template <typename ContainerT, typename PredicateT>
  void remove_if(ContainerT& ioItems, const PredicateT& iPredicate)
  {
    for (auto it = ioItems.begin(); it != ioItems.end();)
    {
      if (iPredicate(*it))
        it = ioItems.erase(it);
      else
        ++it;
    }
  }
}
