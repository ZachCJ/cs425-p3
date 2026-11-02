#include "lab.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#ifdef TEST
#define main main_exclude
#endif

/**
 * @file main.c
 * @author Zachary Johnston
 * @date 10-30-2026
 * @version 1.0
 *
 * @brief Command line interface for IPv4 and CIDR subnet operations and routing table lookup.
 */

/**
 * @brief Print command line usage message.
 *
 * @param out Output stream (stdout or stderr).
 */
static void print_usage(FILE *out) {
  fprintf(out, "Usage: myapp info <address>/<prefix>\n"
               "       myapp split <network>/<prefix> <count>\n"
               "       myapp lookup <table-file> <address>...\n");
}

/**
 * @brief Handle the 'info' command.
 *
 * @param cidr_str Dotted IPv4 address with prefix length (e.g. 132.178.227.11/25).
 * @return 0 on success, 1 on failure.
 */
static int handle_info(const char *cidr_str) {
  cidr_t block;
  if (!cidr_parse(cidr_str, &block)) {
    fprintf(stderr, "myapp: '%s' is not a valid address/prefix\n", cidr_str);
    return 1;
  }

  char addr_buf[IP_STR_LEN];
  char mask_buf[IP_STR_LEN];
  char net_buf[IP_STR_LEN];
  char bcast_buf[IP_STR_LEN];
  char first_buf[IP_STR_LEN];
  char last_buf[IP_STR_LEN];

  ip_format(block.addr, addr_buf);
  ip_format(cidr_mask(block.prefix), mask_buf);
  ip_format(cidr_network(block), net_buf);
  ip_format(cidr_broadcast(block), bcast_buf);
  ip_format(cidr_first_host(block), first_buf);
  ip_format(cidr_last_host(block), last_buf);

  printf("%-10s %s\n", "address", addr_buf);
  printf("%-10s /%u\n", "prefix", block.prefix);
  printf("%-10s %s\n", "netmask", mask_buf);
  printf("%-10s %s\n", "network", net_buf);
  printf("%-10s %s\n", "broadcast", bcast_buf);
  printf("%-10s %s\n", "first host", first_buf);
  printf("%-10s %s\n", "last host", last_buf);
  printf("%-10s %llu\n", "hosts", (unsigned long long)cidr_host_count(block));

  return 0;
}

/**
 * @brief Handle the 'split' command.
 *
 * @param cidr_str Network address with prefix length (e.g. 192.168.1.0/24).
 * @param count_str Number of subnets to create (power of two).
 * @return 0 on success, 1 on failure.
 */
static int handle_split(const char *cidr_str, const char *count_str) {
  cidr_t block;
  if (!cidr_parse(cidr_str, &block)) {
    fprintf(stderr, "myapp: '%s' is not a valid network/prefix\n", cidr_str);
    return 1;
  }

  if (!cidr_is_network(block)) {
    fprintf(stderr, "myapp: %s is not a network (host bits set)\n", cidr_str);
    return 1;
  }

  if (!count_str || *count_str == '\0') {
    fprintf(stderr, "myapp: invalid count\n");
    return 1;
  }

  for (const char *p = count_str; *p; p++) {
    if (*p < '0' || *p > '9') {
      fprintf(stderr, "myapp: invalid count '%s'\n", count_str);
      return 1;
    }
  }

  char *endptr = NULL;
  unsigned long long count_ull = strtoull(count_str, &endptr, 10);
  if (!endptr || *endptr != '\0' || count_ull == 0 || count_ull > 0xFFFFFFFFULL) {
    fprintf(stderr, "myapp: count out of range\n");
    return 1;
  }

  uint32_t count = (uint32_t)count_ull;

  if ((count & (count - 1)) != 0) {
    fprintf(stderr, "myapp: count must be a power of two\n");
    return 1;
  }

  cidr_t test_sub;
  if (!cidr_subnet(block, count, 0, &test_sub)) {
    fprintf(stderr, "myapp: cannot split %s into %u subnets\n", cidr_str, count);
    return 1;
  }

  for (uint32_t i = 0; i < count; i++) {
    cidr_t sub;
    cidr_subnet(block, count, i, &sub);
    char buf[CIDR_STR_LEN];
    cidr_format(sub, buf);
    printf("%s\n", buf);
  }

  return 0;
}

/**
 * @brief Handle the 'lookup' command.
 *
 * @param filename Forwarding table file path.
 * @param argc Total argument count.
 * @param argv Argument vector starting from argv[3] as target addresses.
 * @return 0 on success, 1 on malformed address, 2 on file error, 3 on unreachable route.
 */
static int handle_lookup(const char *filename, int argc, char *argv[]) {
  FILE *f = fopen(filename, "r");
  if (!f) {
    fprintf(stderr, "myapp: %s: cannot open file: %s\n", filename, strerror(errno));
    return 2;
  }

  fwd_table_t *table = table_create();
  if (!table) {
    fclose(f);
    fprintf(stderr, "myapp: failed to allocate forwarding table\n");
    return 1;
  }

  size_t bad_line = 0;
  if (!table_load(table, f, &bad_line)) {
    fclose(f);
    table_destroy(table);
    if (bad_line > 0) {
      fprintf(stderr, "myapp: %s:%zu: not a valid route\n", filename, bad_line);
    } else {
      fprintf(stderr, "myapp: %s: could not read table\n", filename);
    }
    return 2;
  }
  fclose(f);

  bool has_malformed = false;
  bool has_unreachable = false;

  for (int i = 3; i < argc; i++) {
    uint32_t addr = 0;
    if (!ip_parse(argv[i], &addr)) {
      fprintf(stderr, "myapp: %s: malformed address\n", argv[i]);
      has_malformed = true;
      continue;
    }

    const route_t *r = table_lookup(table, addr);
    if (!r) {
      printf("%s unreachable\n", argv[i]);
      has_unreachable = true;
    } else if (r->gateway == 0) {
      printf("%s dev %s\n", argv[i], r->dev);
    } else {
      char gw_buf[IP_STR_LEN];
      ip_format(r->gateway, gw_buf);
      printf("%s via %s dev %s\n", argv[i], gw_buf, r->dev);
    }
  }

  table_destroy(table);

  if (has_malformed) {
    return 1;
  }
  if (has_unreachable) {
    return 3;
  }
  return 0;
}

/**
 * @brief Application entry point.
 */
int main(int argc, char *argv[]) {
  if (argc <= 1) {
    print_usage(stdout);
    return 0;
  }

  if (strcmp(argv[1], "info") == 0) {
    if (argc != 3) {
      fprintf(stderr, "myapp: 'info' requires exactly one <address>/<prefix> argument\n");
      print_usage(stderr);
      return 1;
    }
    return handle_info(argv[2]);
  }

  if (strcmp(argv[1], "split") == 0) {
    if (argc != 4) {
      fprintf(stderr, "myapp: 'split' requires <network>/<prefix> and <count>\n");
      print_usage(stderr);
      return 1;
    }
    return handle_split(argv[2], argv[3]);
  }

  if (strcmp(argv[1], "lookup") == 0) {
    if (argc < 4) {
      fprintf(stderr, "myapp: 'lookup' requires <table-file> and at least one <address>\n");
      print_usage(stderr);
      return 1;
    }
    return handle_lookup(argv[2], argc, argv);
  }

  fprintf(stderr, "myapp: unknown command '%s'\n", argv[1]);
  print_usage(stderr);
  return 1;
}
