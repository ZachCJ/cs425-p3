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

/** Internal structure for the forwarding table. */
struct fwd_table {
  route_t *routes;
  size_t count;
  size_t capacity;
};

/**
 * @brief Parse one line of a forwarding table file.
 *
 * @param line The line.
 * @param route Where the route is stored on success; untouched on failure.
 * @return true if the line is a valid route, false otherwise (including NULL).
 */
bool route_parse(const char *line, route_t *route) {
  if (!line || !route) {
    return false;
  }

  const char *p = line;

  // 1. Skip leading whitespace
  while (*p && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) {
    p++;
  }
  if (*p == '\0') {
    return false;
  }

  // 2. Field 1: Destination
  const char *f1_start = p;
  while (*p && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') {
    p++;
  }
  size_t f1_len = (size_t)(p - f1_start);

  // Skip whitespace between field 1 and field 2
  while (*p && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) {
    p++;
  }
  if (*p == '\0') {
    return false;
  }

  // 3. Field 2: Gateway
  const char *f2_start = p;
  while (*p && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') {
    p++;
  }
  size_t f2_len = (size_t)(p - f2_start);

  // Skip whitespace between field 2 and field 3
  while (*p && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) {
    p++;
  }
  if (*p == '\0') {
    return false;
  }

  // 4. Field 3: Device
  const char *f3_start = p;
  while (*p && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') {
    p++;
  }
  size_t f3_len = (size_t)(p - f3_start);

  // Skip trailing whitespace
  while (*p && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) {
    p++;
  }
  if (*p != '\0') {
    return false;
  }

  // Length sanity checks
  char token1[64];
  char token2[64];
  char token3[64];

  if (f1_len >= sizeof(token1) || f2_len >= sizeof(token2) || f3_len >= sizeof(token3)) {
    return false;
  }

  memcpy(token1, f1_start, f1_len);
  token1[f1_len] = '\0';

  memcpy(token2, f2_start, f2_len);
  token2[f2_len] = '\0';

  memcpy(token3, f3_start, f3_len);
  token3[f3_len] = '\0';

  // Parse destination: either "default" (0.0.0.0/0) or CIDR block with no host bits set
  cidr_t dest;
  if (strcmp(token1, "default") == 0) {
    dest.addr = 0;
    dest.prefix = 0;
  } else {
    if (!cidr_parse(token1, &dest)) {
      return false;
    }
    if (!cidr_is_network(dest)) {
      return false;
    }
  }

  // Parse gateway: either "direct" (0) or dotted quad IP
  uint32_t gateway = 0;
  if (strcmp(token2, "direct") == 0) {
    gateway = 0;
  } else {
    if (!ip_parse(token2, &gateway)) {
      return false;
    }
  }

  // Device must be 1 to DEV_NAME_LEN - 1 characters
  if (f3_len == 0 || f3_len >= DEV_NAME_LEN) {
    return false;
  }

  // Success: write to route
  route->dest = dest;
  route->gateway = gateway;
  memset(route->dev, 0, sizeof(route->dev));
  memcpy(route->dev, token3, f3_len);
  route->dev[f3_len] = '\0';

  return true;
}

/**
 * @brief Create an empty forwarding table.
 * @return The table, which the caller frees with table_destroy(), or NULL if
 *         memory could not be allocated.
 */
fwd_table_t *table_create(void) {
  fwd_table_t *table = (fwd_table_t *)malloc(sizeof(fwd_table_t));
  if (!table) {
    return NULL;
  }
  table->routes = NULL;
  table->count = 0;
  table->capacity = 0;
  return table;
}

/** @brief Free a table and everything in it. NULL is allowed and does nothing. */
void table_destroy(fwd_table_t *table) {
  if (!table) {
    return;
  }
  free(table->routes);
  free(table);
}

/** @brief The number of routes in the table; 0 for NULL. */
size_t table_count(const fwd_table_t *table) {
  if (!table) {
    return 0;
  }
  return table->count;
}

/**
 * @brief Add a copy of a route to the table, growing it as needed.
 * @return false if either argument is NULL, the destination has host bits
 *         set, the table already holds a route to the same block, or memory
 *         could not be allocated. The table is unchanged on failure.
 */
bool table_add(fwd_table_t *table, const route_t *route) {
  if (!table || !route) {
    return false;
  }

  // Destination cannot have host bits set, and prefix cannot exceed 32
  if (route->dest.prefix > 32 || !cidr_is_network(route->dest)) {
    return false;
  }

  // Table cannot already hold a route to the same block
  for (size_t idx = 0; idx < table->count; idx++) {
    if (table->routes[idx].dest.addr == route->dest.addr &&
        table->routes[idx].dest.prefix == route->dest.prefix) {
      return false;
    }
  }

  // Grow array if capacity reached
  if (table->count >= table->capacity) {
    size_t new_cap = table->capacity == 0 ? 8 : table->capacity * 2;
    route_t *new_routes = (route_t *)realloc(table->routes, new_cap * sizeof(route_t));
    if (!new_routes) {
      return false;
    }
    table->routes = new_routes;
    table->capacity = new_cap;
  }

  table->routes[table->count] = *route;
  table->count++;
  return true;
}

/**
 * @brief Helper to read a line of arbitrary length from a file stream.
 *
 * @param in Open file stream.
 * @return Heap-allocated string which caller must free, or NULL on EOF / error.
 */
static char *table_read_line(FILE *in) {
  size_t cap = 128;
  size_t len = 0;
  char *buf = (char *)malloc(cap);
  if (!buf) {
    return NULL;
  }

  int c = fgetc(in);
  if (c == EOF) {
    free(buf);
    return NULL;
  }

  while (c != EOF && c != '\n') {
    if (len + 1 >= cap) {
      size_t new_cap = cap * 2;
      char *new_buf = (char *)realloc(buf, new_cap);
      if (!new_buf) {
        free(buf);
        return NULL;
      }
      buf = new_buf;
      cap = new_cap;
    }
    buf[len++] = (char)c;
    c = fgetc(in);
  }
  buf[len] = '\0';
  return buf;
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
  if (!table || !in) {
    if (bad_line) {
      *bad_line = 0;
    }
    return false;
  }

  size_t line_num = 0;
  char *line = NULL;

  while ((line = table_read_line(in)) != NULL) {
    line_num++;

    // Everything from '#' to end of line is a comment
    char *comment = strchr(line, '#');
    if (comment) {
      *comment = '\0';
    }

    // Check if line is blank once comment is removed
    const char *p = line;
    while (*p && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) {
      p++;
    }
    if (*p == '\0') {
      free(line);
      continue;
    }

    // Parse route
    route_t r;
    if (!route_parse(line, &r)) {
      free(line);
      if (bad_line) {
        *bad_line = line_num;
      }
      return false;
    }

    // Add route to table
    if (!table_add(table, &r)) {
      free(line);
      if (bad_line) {
        *bad_line = line_num;
      }
      return false;
    }

    free(line);
  }

  return true;
}

/**
 * @brief Longest prefix match: the route whose block contains dest and has the
 *        longest prefix.
 *
 * @return The matching route, which stays owned by the table, or NULL if no
 *         route matches (or table is NULL).
 */
const route_t *table_lookup(const fwd_table_t *table, uint32_t dest) {
  if (!table || table->count == 0) {
    return NULL;
  }

  const route_t *best_match = NULL;

  for (size_t idx = 0; idx < table->count; idx++) {
    if (cidr_contains(table->routes[idx].dest, dest)) {
      if (!best_match || table->routes[idx].dest.prefix > best_match->dest.prefix) {
        best_match = &table->routes[idx];
      }
    }
  }

  return best_match;
}
