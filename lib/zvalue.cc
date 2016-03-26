#include "zvalue.h"

namespace Zoe {


ZInteger::ZInteger(vector<uint8_t> const& data, size_t n)
    : _value(static_cast<int64_t>(data[n])         | \
             static_cast<int64_t>(data[n+1]) << 8  | \
             static_cast<int64_t>(data[n+2]) << 16 | \
             static_cast<int64_t>(data[n+3]) << 24 | \
             static_cast<int64_t>(data[n+4]) << 32 | \
             static_cast<int64_t>(data[n+5]) << 40 | \
             static_cast<int64_t>(data[n+6]) << 48 | \
             static_cast<int64_t>(data[n+7]) << 56)
{
}


}

// vim: ts=4:sw=4:sts=4:expandtab:foldmethod=marker
