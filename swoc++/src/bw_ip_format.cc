/** @file

  BufferWriter formatting for IP address data.

  @section license License

  Licensed to the Apache Software Foundation (ASF) under one or more contributor license agreements.
  See the NOTICE file distributed with this work for additional information regarding copyright
  ownership.  The ASF licenses this file to you under the Apache License, Version 2.0 (the
  "License"); you may not use this file except in compliance with the License.  You may obtain a
  copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software distributed under the License
  is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
  or implied. See the License for the specific language governing permissions and limitations under
  the License.
 */

#include "swoc/swoc_ip.h"
#include "swoc/bwf_ip.h"

using namespace swoc::literals;

namespace
{
std::string_view
family_name(sa_family_t family)
{
  switch (family) {
  case AF_INET:
    return "ipv4"_sv;
  case AF_INET6:
    return "ipv6"_sv;
  case AF_UNIX:
    return "unix"_sv;
  case AF_UNSPEC:
    return "unspec"_sv;
  default:
    return "unknown"_sv;
  }
}

} // namespace

namespace swoc
{
using bwf::Spec;

BufferWriter &
bwformat(BufferWriter &w, Spec const &spec, IP4Addr const& addr)
{
  in_addr_t host = addr.host_order();
  Spec local_spec{spec}; // Format for address elements.
  bool align_p = false;

  if (spec._ext.size()) {
    if (spec._ext.front() == '=') {
      align_p          = true;
      local_spec._fill = '0';
    } else if (spec._ext.size() > 1 && spec._ext[1] == '=') {
      align_p          = true;
      local_spec._fill = spec._ext[0];
    }
  }

  if (align_p) {
    local_spec._min   = 3;
    local_spec._align = Spec::Align::RIGHT;
  } else {
    local_spec._min = 0;
  }

  bwformat(w, local_spec, static_cast<uint8_t>(host >> 24 & 0xFF));
  w.write('.');
  bwformat(w, local_spec, static_cast<uint8_t>(host >> 16 & 0xFF));
  w.write('.');
  bwformat(w, local_spec, static_cast<uint8_t>(host >> 8 & 0xFF));
  w.write('.');
  bwformat(w, local_spec, static_cast<uint8_t>(host & 0xFF));
  return w;
}

BufferWriter &
bwformat(BufferWriter &w, Spec const &spec, in6_addr const &addr)
{
  using QUAD = uint16_t const;
  Spec local_spec{spec}; // Format for address elements.
  uint8_t const *ptr   = addr.s6_addr;
  uint8_t const *limit = ptr + sizeof(addr.s6_addr);
  QUAD *lower          = nullptr; // the best zero range
  QUAD *upper          = nullptr;
  bool align_p         = false;

  if (spec._ext.size()) {
    if (spec._ext.front() == '=') {
      align_p          = true;
      local_spec._fill = '0';
    } else if (spec._ext.size() > 1 && spec._ext[1] == '=') {
      align_p          = true;
      local_spec._fill = spec._ext[0];
    }
  }

  if (align_p) {
    local_spec._min   = 4;
    local_spec._align = Spec::Align::RIGHT;
  } else {
    local_spec._min = 0;
    // do 0 compression if there's no internal fill.
    for (QUAD *spot = reinterpret_cast<QUAD *>(ptr), *last = reinterpret_cast<QUAD *>(limit), *current = nullptr; spot < last;
         ++spot) {
      if (0 == *spot) {
        if (current) {
          // If there's no best, or this is better, remember it.
          if (!lower || (upper - lower < spot - current)) {
            lower = current;
            upper = spot;
          }
        } else {
          current = spot;
        }
      } else {
        current = nullptr;
      }
    }
  }

  if (!local_spec.has_numeric_type()) {
    local_spec._type = 'x';
  }

  for (; ptr < limit; ptr += 2) {
    if (reinterpret_cast<uint8_t const *>(lower) <= ptr && ptr <= reinterpret_cast<uint8_t const *>(upper)) {
      if (ptr == addr.s6_addr) {
        w.write(':'); // only if this is the first quad.
      }
      if (ptr == reinterpret_cast<uint8_t const *>(upper)) {
        w.write(':');
      }
    } else {
      uint16_t f = (ptr[0] << 8) + ptr[1];
      bwformat(w, local_spec, f);
      if (ptr != limit - 2) {
        w.write(':');
      }
    }
  }
  return w;
}

BufferWriter &
bwformat(BufferWriter &w, Spec const &spec, IPAddr const &addr)
{
  Spec local_spec{spec}; // Format for address elements and port.
  bool addr_p{true};
  bool family_p{false};

  if (spec._ext.size()) {
    if (spec._ext.front() == '=') {
      local_spec._ext.remove_prefix(1);
    } else if (spec._ext.size() > 1 && spec._ext[1] == '=') {
      local_spec._ext.remove_prefix(2);
    }
  }
  if (local_spec._ext.size()) {
    addr_p = false;
    for (char c : local_spec._ext) {
      switch (c) {
      case 'a':
      case 'A':
        addr_p = true;
        break;
      case 'f':
      case 'F':
        family_p = true;
        break;
      }
    }
  }

  if (addr_p) {
    if (addr.is_ip4()) {
      swoc::bwformat(w, spec, addr.network_ip4());
    } else if (addr.is_ip6()) {
      swoc::bwformat(w, spec, addr.network_ip6());
    } else {
      w.print("*Not IP address [{}]*", addr.family());
    }
  }

  if (family_p) {
    local_spec._min = 0;
    if (addr_p) {
      w.write(' ');
    }
    if (spec.has_numeric_type()) {
      bwformat(w, local_spec, static_cast<uintmax_t>(addr.family()));
    } else {
      swoc::bwformat(w, local_spec, addr.family());
    }
  }
  return w;
}

BufferWriter &
bwformat(BufferWriter &w, Spec const &spec, sockaddr const *addr)
{
  Spec local_spec{spec}; // Format for address elements and port.
  bool port_p{true};
  bool addr_p{true};
  bool family_p{false};
  bool local_numeric_fill_p{false};
  char local_numeric_fill_char{'0'};

  if (spec._type == 'p' || spec._type == 'P') {
    bwformat(w, spec, static_cast<void const *>(addr));
    return w;
  }

  if (spec._ext.size()) {
    if (spec._ext.front() == '=') {
      local_numeric_fill_p = true;
      local_spec._ext.remove_prefix(1);
    } else if (spec._ext.size() > 1 && spec._ext[1] == '=') {
      local_numeric_fill_p    = true;
      local_numeric_fill_char = spec._ext.front();
      local_spec._ext.remove_prefix(2);
    }
  }
  if (local_spec._ext.size()) {
    addr_p = port_p = false;
    for (char c : local_spec._ext) {
      switch (c) {
      case 'a':
      case 'A':
        addr_p = true;
        break;
      case 'p':
      case 'P':
        port_p = true;
        break;
      case 'f':
      case 'F':
        family_p = true;
        break;
      }
    }
  }

  if (addr_p) {
    bool bracket_p = false;
    switch (addr->sa_family) {
    case AF_INET:
      bwformat(w, spec, IP4Addr{reinterpret_cast<sockaddr_in const *>(addr)->sin_addr.s_addr});
      break;
    case AF_INET6:
      if (port_p) {
        w.write('[');
        bracket_p = true; // take a note - put in the trailing bracket.
      }
      bwformat(w, spec, reinterpret_cast<sockaddr_in6 const *>(addr)->sin6_addr);
      break;
    default:
      w.print("*Not IP address [{}]*", addr->sa_family);
      break;
    }
    if (bracket_p)
      w.write(']');
    if (port_p)
      w.write(':');
  }
  if (port_p) {
    if (local_numeric_fill_p) {
      local_spec._min   = 5;
      local_spec._fill  = local_numeric_fill_char;
      local_spec._align = Spec::Align::RIGHT;
    } else {
      local_spec._min = 0;
    }
    bwformat(w, local_spec, static_cast<uintmax_t>(IPEndpoint::host_order_port(addr)));
  }
  if (family_p) {
    local_spec._min = 0;
    if (addr_p || port_p)
      w.write(' ');
    if (spec.has_numeric_type()) {
      bwformat(w, local_spec, static_cast<uintmax_t>(addr->sa_family));
    } else {
      swoc::bwformat(w, local_spec, IPEndpoint::family_name(addr->sa_family));
    }
  }
  return w;
}

} // namespace swoc
