#include "lab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @file lab.c
 * @author Zachary Johnston
 * @date 10-30-2026
 * @version 1.0 
 * 
 * @brief 
 * 
 * @note - Full Documentation on Lab.h this file sticks to briefs
 */

/* ------------------------------------------------------------------------
 * Task 1 - Address parsing and formatting
 * --------------------------------------------------------------------- */

/**
 * @brief Helper to parse a single decimal octet from a substring [start, end).
 *
 * An octet must be 1 to 3 decimal digits, representing a value from 0 to 255.
 * Leading zeros are rejected (e.g. "01", "00", "092"), though a single "0" is valid.
 *
 * @param start Pointer to start of octet.
 * @param end Pointer to character immediately following octet.
 * @param octet Where to store the parsed octet value.
 * @return true if valid octet, false otherwise.
 */
static bool parse_octet_slice(const char *start, const char *end, uint8_t *octet) {
  if (!start || !end || end <= start) {
    return false;
  }
  size_t len = (size_t)(end - start);
  if (len > 3) {
    return false;
  }
  if (len > 1 && start[0] == '0') {
    return false;
  }
  unsigned int val = 0;
  for (size_t i = 0; i < len; i++) {
    if (start[i] < '0' || start[i] > '9') {
      return false;
    }
    val = val * 10 + (unsigned int)(start[i] - '0');
  }
  if (val > 255) {
    return false;
  }
  *octet = (uint8_t)val;
  return true;
}

/**
 * @brief Parse a dotted quad such as "192.168.1.10".
 *
 * @param text The string to parse.
 * @param addr Where the address is stored on success; untouched on failure.
 * @return true if text is a valid dotted quad, false otherwise (including NULL).
 */
bool ip_parse(const char *text, uint32_t *addr) {
  if (!text || !addr) {
    return false;
  }

  uint32_t result = 0;
  const char *p = text;

  for (int i = 0; i < 4; i++) {
    const char *start = p;
    const char *end = NULL;

    if (i < 3) {
      end = strchr(p, '.');
      if (!end) {
        return false;
      }
      p = end + 1;
    } else {
      end = start + strlen(start);
    }

    uint8_t octet = 0;
    if (!parse_octet_slice(start, end, &octet)) {
      return false;
    }

    result = (result << 8) | octet;
  }

  *addr = result;
  return true;
}

/**
 * @brief Format an address as a dotted quad.
 * @param addr The address, host byte order.
 * @param out A buffer of at least IP_STR_LEN bytes.
 */
void ip_format(uint32_t addr, char out[IP_STR_LEN]) {
  if (!out) {
    return;
  }
  snprintf(out, IP_STR_LEN, "%u.%u.%u.%u",
           (addr >> 24) & 0xFF,
           (addr >> 16) & 0xFF,
           (addr >> 8) & 0xFF,
           addr & 0xFF);
}

/**
 * @brief Parse a block in CIDR notation such as "192.168.1.0/24".
 *
 * @param text The string to parse.
 * @param block Where the block is stored on success; untouched on failure.
 * @return true if text is valid CIDR notation, false otherwise (including NULL).
 */
bool cidr_parse(const char *text, cidr_t *block) {
  if (!text || !block) {
    return false;
  }

  const char *slash = strchr(text, '/');
  if (!slash) {
    return false;
  }

  // Parse prefix after '/'
  const char *prefix_str = slash + 1;
  if (*prefix_str == '\0') {
    return false;
  }

  size_t plen = strlen(prefix_str);
  if (plen > 2) {
    return false;
  }
  if (plen > 1 && prefix_str[0] == '0') {
    return false; // Leading zero (e.g. /024)
  }
  unsigned int prefix_val = 0;
  for (size_t i = 0; i < plen; i++) {
    if (prefix_str[i] < '0' || prefix_str[i] > '9') {
      return false;
    }
    prefix_val = prefix_val * 10 + (unsigned int)(prefix_str[i] - '0');
  }
  if (prefix_val > 32) {
    return false;
  }

  // Parse IP part before '/'
  uint32_t result = 0;
  const char *p = text;

  for (int i = 0; i < 4; i++) {
    const char *start = p;
    const char *end = NULL;

    if (i < 3) {
      end = strchr(p, '.');
      if (!end || end >= slash) {
        return false;
      }
      p = end + 1;
    } else {
      end = slash;
    }

    uint8_t octet = 0;
    if (!parse_octet_slice(start, end, &octet)) {
      return false;
    }

    result = (result << 8) | octet;
  }

  block->addr = result;
  block->prefix = (uint8_t)prefix_val;
  return true;
}

/**
 * @brief Format a block as "a.b.c.d/p", using the address exactly as stored.
 *
 * @param block The block.
 * @param out A buffer of at least CIDR_STR_LEN bytes.
 */
void cidr_format(cidr_t block, char out[CIDR_STR_LEN]) {
  if (!out) {
    return;
  }
  uint8_t prefix = block.prefix > 32 ? 32 : block.prefix;
  snprintf(out, CIDR_STR_LEN, "%u.%u.%u.%u/%u",
           (block.addr >> 24) & 0xFF,
           (block.addr >> 16) & 0xFF,
           (block.addr >> 8) & 0xFF,
           block.addr & 0xFF,
           prefix);
}

/* ------------------------------------------------------------------------
 * Task 2 - Subnet arithmetic
 * --------------------------------------------------------------------- */

/**
 * @brief The subnet mask for a prefix length: /24 is 255.255.255.0.
 *
 * @param prefix 0 to 32; anything larger is treated as 32.
 * @return The mask, host byte order.
 */
uint32_t cidr_mask(uint8_t prefix) {
  if (prefix == 0) {
    return 0;
  }
  if (prefix >= 32) {
    return 0xFFFFFFFFU;
  }
  return 0xFFFFFFFFU << (32 - prefix);
}

/** @brief The lowest address in the block: the address with the host bits cleared. */
uint32_t cidr_network(cidr_t block) {
  return block.addr & cidr_mask(block.prefix);
}

/** @brief The highest address in the block: the address with the host bits set. */
uint32_t cidr_broadcast(cidr_t block) {
  return block.addr | ~cidr_mask(block.prefix);
}

/**
 * @brief The first address that can be given to a host.
 *
 */
uint32_t cidr_first_host(cidr_t block) {
  uint32_t net = cidr_network(block);
  if (block.prefix >= 31) {
    return net;
  }
  return net + 1;
}

/**
 * @brief The last address that can be given to a host.
 *
 */
uint32_t cidr_last_host(cidr_t block) {
  uint32_t bcast = cidr_broadcast(block);
  if (block.prefix >= 31) {
    return bcast;
  }
  return bcast - 1;
}

/**
 * @brief How many addresses in the block can be given to hosts.
 *
 */
uint64_t cidr_host_count(cidr_t block) {
  if (block.prefix >= 32) {
    return 1;
  }
  if (block.prefix == 31) {
    return 2;
  }
  return (1ULL << (32 - block.prefix)) - 2;
}

/** @brief true if addr falls inside the block. */
bool cidr_contains(cidr_t block, uint32_t addr) {
  uint32_t mask = cidr_mask(block.prefix);
  return (addr & mask) == (block.addr & mask);
}

/** @brief true if the block's host bits are all zero, so it names a network. */
bool cidr_is_network(cidr_t block) {
  uint32_t mask = cidr_mask(block.prefix);
  return (block.addr & ~mask) == 0;
}

/**
 * @brief The i-th of n equally sized subnets of a block.
 *
 * @param block The block to split; it must be a network (cidr_is_network()).
 * @param n How many subnets, a power of two from 1 up.
 * @param i Which subnet, 0 to n - 1.
 * @param out Where the subnet is stored on success; untouched on failure.
 * @return false if out is NULL, the block is not a network, n is not a power
 *         of two, the split would need a prefix longer than 32, or i is out of
 *         range.
 */
bool cidr_subnet(cidr_t block, uint32_t n, uint32_t i, cidr_t *out) {
  if (!out || !cidr_is_network(block) || n == 0) {
    return false;
  }

  // n must be a power of two
  if ((n & (n - 1)) != 0) {
    return false;
  }

  // i must be in range 0 to n - 1
  if (i >= n) {
    return false;
  }

  // Determine number of borrowed bits k = log2(n)
  uint32_t temp = n;
  uint8_t k = 0;
  while (temp > 1) {
    temp >>= 1;
    k++;
  }

  uint32_t new_prefix = (uint32_t)block.prefix + k;
  if (new_prefix > 32) {
    return false;
  }

  uint32_t addr = block.addr;
  if (new_prefix > 0) {
    addr += (i << (32 - new_prefix));
  }

  out->addr = addr;
  out->prefix = (uint8_t)new_prefix;
  return true;
}

/* ------------------------------------------------------------------------
 * Task 3 - Longest prefix match
 * --------------------------------------------------------------------- */

/**
 * @brief Parse one line of a forwarding table file.
 *
 * @param line The line.
 * @param route Where the route is stored on success; untouched on failure.
 * @return true if the line is a valid route, false otherwise (including NULL).
 */
bool route_parse(const char *line, route_t *route) {
  (void)line;
  (void)route;
  return false;
}

/**
 * @brief Create an empty forwarding table.
 * @return The table, which the caller frees with table_destroy(), or NULL if
 *         memory could not be allocated.
 */
fwd_table_t *table_create(void) {
  return NULL;
}

/** @brief Free a table and everything in it. NULL is allowed and does nothing. */
void table_destroy(fwd_table_t *table) {
  (void)table;
}

/** @brief The number of routes in the table; 0 for NULL. */
size_t table_count(const fwd_table_t *table) {
  (void)table;
  return 0;
}

/**
 * @brief Add a copy of a route to the table, growing it as needed.
 * @return false if either argument is NULL, the destination has host bits
 *         set, the table already holds a route to the same block, or memory
 *         could not be allocated. The table is unchanged on failure.
 */
bool table_add(fwd_table_t *table, const route_t *route) {
  (void)table;
  (void)route;
  return false;
}

/**
 * @brief Read a forwarding table file and add every route in it.
 *
 * @param table The table to add to.
 * @param in The open file to read.
 * @param bad_line If not NULL, set to the 1-based number of the first line
 *        that failed, or 0 if the arguments were NULL or the file could not
 *        be read.
 * @return true if every line loaded. On false, routes from the lines before
 *         the bad one stay in the table.
 */
bool table_load(fwd_table_t *table, FILE *in, size_t *bad_line) {
  (void)table;
  (void)in;
  (void)bad_line;
  return false;
}

/**
 * @brief Longest prefix match: the route whose block contains dest and has the
 *        longest prefix.
 *
 * @return The matching route, which stays owned by the table, or NULL if no
 *         route matches (or table is NULL).
 */
const route_t *table_lookup(const fwd_table_t *table, uint32_t dest) {
  (void)table;
  (void)dest;
  return NULL;
}
