#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "harness/unity.h"
#include "../src/lab.h"

/**
 * @file lab-test.c
 * @author Zachary Johnston
 * @date 10-30-2026
 * @version 1.0 
 * 
 * @details These are all of the tests for lab.c implementation. 
 *    The tests for the methods are ordered and organized the same as lab.h for consistency.
 *    Any values that are constant are expected to be unchanging.
 *    "exp" is an abbreviation for expected.
 */

// Text Color Formatting Escape Characters
#define RED   "\033[0;31m"
#define BLUE    "\x1b[34m"
#define YELLOW  "\x1b[33m"
#define CYAN    "\x1b[36m"
#define RESET   "\x1b[0m"

void setUp(void) {
  printf("Setting up tests...\n");
}

void tearDown(void) {
  printf("Tearing down tests...\n");
}

/**
 * @brief Helper to construct an IPv4 address in host byte order.
 */
static inline uint32_t make_ip(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
  return ((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)c << 8) | (uint32_t)d;
}

/**
 * @brief Helper to construct a cidr_t block.
 */
static inline cidr_t make_cidr(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t prefix) {
  cidr_t block;
  block.addr = make_ip(a, b, c, d);
  block.prefix = prefix;
  return block;
}

/**
 * @brief Helper to construct a route_t struct.
 */
static inline route_t make_route(cidr_t dest, uint32_t gateway, const char *dev) {
  route_t r;
  r.dest = dest;
  r.gateway = gateway;
  memset(r.dev, 0, sizeof(r.dev));
  if (dev) {
    strncpy(r.dev, dev, sizeof(r.dev) - 1);
  }
  return r;
}

/**
 * @brief Helper to create a temporary file populated with content.
 */
static FILE *create_temp_file(const char *content) {
  FILE *f = tmpfile();
  if (!f) {
    f = fopen("build/test_table_tmp.txt", "w+b");
  }
  if (f && content) {
    fputs(content, f);
    rewind(f);
  }
  return f;
}

//--------------------------------//
//      Task 1 Method Tests       //
//--------------------------------//

// Note: I wrote these task 1 tests before using AI

void ip_parse_functionality_test(void) {
  //Initialize Data for the tests
  const char * ipText = "192.168.1.1";

  const uint8_t octet1 = 192;
  const uint8_t octet2 = 168;
  const uint8_t octet3 = 1;
  const uint8_t octet4 = 1;

  const uint32_t expAddr = ((uint32_t)octet1 << 24) |
                       ((uint32_t)octet2 << 16) |
                       ((uint32_t)octet3 << 8)  |
                       (uint32_t)octet4;
  
  const char * badIpText = "092.168.1.1";
  const char * invalidFormatText = "This should fail";
  
  const char * mustRejectIPs[10] = {
    "1.2.3",        "1.2.3.4.5",    "1..2.3",       "256.1.1.1",    "010.0.0.1",
    "1.2.3.4x",     "1.2.3.4/33",   "1.2.3.4/024",  "1.2.3.4/",     "1.2.3/24"
  };

  uint32_t * returnAddr = (uint32_t *)malloc(sizeof(uint32_t));
  if(!returnAddr) {
    printf(RED "Malloc failed for returnAddr" RESET);
    return;
  }
  bool returnStatus = true;

  //Tests
  //Valid Use Case
  returnStatus = ip_parse(ipText, returnAddr);

  TEST_ASSERT_TRUE(returnStatus);
  TEST_ASSERT_EQUAL_UINT32(expAddr, *returnAddr);

  //Invalid Case: bad IP i.e. leading zero
  returnStatus = ip_parse(badIpText, returnAddr);
  TEST_ASSERT_FALSE(returnStatus);
  TEST_ASSERT_EQUAL_UINT32(expAddr, *returnAddr); //Kept because returnAddr should be unchanged

  //Invalid Case: bad format i.e. not an IP address
  returnStatus = ip_parse(invalidFormatText, returnAddr);
  TEST_ASSERT_FALSE(returnStatus);
  TEST_ASSERT_EQUAL_UINT32(expAddr, *returnAddr); //Kept because returnAddr should be unchanged

  //Invalid Cases: Ones to check from Canvas Assignment
  for(int i = 0; i < 10; i++) {
    returnStatus = ip_parse(mustRejectIPs[i], returnAddr);
    TEST_ASSERT_FALSE(returnStatus);
    TEST_ASSERT_EQUAL_UINT32(expAddr, *returnAddr); //Kept because returnAddr should be unchanged
  }

  // NULL tests
  TEST_ASSERT_FALSE(ip_parse(NULL, returnAddr));
  TEST_ASSERT_EQUAL_UINT32(expAddr, *returnAddr);
  TEST_ASSERT_FALSE(ip_parse(ipText, NULL));

  // Boundary checks: 0.0.0.0 and 255.255.255.255
  TEST_ASSERT_TRUE(ip_parse("0.0.0.0", returnAddr));
  TEST_ASSERT_EQUAL_UINT32(0, *returnAddr);

  TEST_ASSERT_TRUE(ip_parse("255.255.255.255", returnAddr));
  TEST_ASSERT_EQUAL_UINT32(0xFFFFFFFF, *returnAddr);

  //Cleanup
  free(returnAddr);
}

void ip_format_functionality_test(void) {
  //Initialize Data for the tests
  const char * expIPText = "192.168.1.1";

  const uint8_t octet1 = 192;
  const uint8_t octet2 = 168;
  const uint8_t octet3 = 1;
  const uint8_t octet4 = 1;

  const uint32_t inAddr = ((uint32_t)octet1 << 24) |
                       ((uint32_t)octet2 << 16) |
                       ((uint32_t)octet3 << 8)  |
                       (uint32_t)octet4;

  char * returnIP = (char *)malloc(sizeof(char) * IP_STR_LEN);
  if(!returnIP) {
    printf(RED "Malloc failed for returnIP" RESET);
    return;
  }

  //Only one case because this should be getting the output of ip_parse
  ip_format(inAddr, returnIP);

  TEST_ASSERT_EQUAL_STRING(expIPText, returnIP);

  // Boundary checks
  ip_format(0, returnIP);
  TEST_ASSERT_EQUAL_STRING("0.0.0.0", returnIP);

  ip_format(0xFFFFFFFF, returnIP);
  TEST_ASSERT_EQUAL_STRING("255.255.255.255", returnIP);

  free(returnIP);
}

void cidr_parse_functionality_test(void) {
   //Initialize Data for the tests
  const char * cidrText = "192.168.1.1/24";

  const uint8_t octet1 = 192;
  const uint8_t octet2 = 168;
  const uint8_t octet3 = 1;
  const uint8_t octet4 = 1;
  const uint8_t expPrefix = 24;
  
  const uint32_t expAddr = ((uint32_t)octet1 << 24) |
                       ((uint32_t)octet2 << 16) |
                       ((uint32_t)octet3 << 8)  |
                       (uint32_t)octet4;
  
  const char * badCIDRText = "192.168.1.1/024";
  const char * invalidFormatText = "This should fail";
  
  const char * mustRejectCIDRs[10] = {
    "1.2.3.4x",     "1.2.3.4/33",   "1.2.3.4/024",  "1.2.3.4/",     "1.2.3/24",
    "1.2.3",        "1.2.3.4.5",    "1..2.3",       "256.1.1.1",    "010.0.0.1"
  };
  cidr_t * actualCIDR = (cidr_t *)malloc(sizeof(cidr_t));
  if(!actualCIDR) {
    printf(RED "Malloc failed for actualCIDR" RESET);
    return;
  }

  bool returnStatus = true;
  //Valid Use Case
  returnStatus = cidr_parse(cidrText, actualCIDR);

  TEST_ASSERT_TRUE(returnStatus);
  TEST_ASSERT_EQUAL_UINT32(expAddr, actualCIDR->addr); 
  TEST_ASSERT_EQUAL_UINT8(expPrefix, actualCIDR->prefix); 

  //Invalid Case: bad CIDR i.e. leading zero in prefix
  returnStatus = cidr_parse(badCIDRText, actualCIDR);
  TEST_ASSERT_FALSE(returnStatus);
  // actualCIDR should be unchanged
  TEST_ASSERT_EQUAL_UINT32(expAddr, actualCIDR->addr); 
  TEST_ASSERT_EQUAL_UINT8(expPrefix, actualCIDR->prefix); 

  //Invalid Case: bad format i.e. not a CIDR address
  returnStatus = cidr_parse(invalidFormatText, actualCIDR);
  TEST_ASSERT_FALSE(returnStatus);
  // actualCIDR should be unchanged
  TEST_ASSERT_EQUAL_UINT32(expAddr, actualCIDR->addr); 
  TEST_ASSERT_EQUAL_UINT8(expPrefix, actualCIDR->prefix); 

  //Invalid Cases: Ones to check from Canvas Assignment
  for(int i = 0; i < 10; i++) {
    returnStatus = cidr_parse(mustRejectCIDRs[i], actualCIDR);
    TEST_ASSERT_FALSE(returnStatus);
    // actualCIDR should be unchanged
    TEST_ASSERT_EQUAL_UINT32(expAddr, actualCIDR->addr); 
    TEST_ASSERT_EQUAL_UINT8(expPrefix, actualCIDR->prefix); 
  }

  // NULL tests
  TEST_ASSERT_FALSE(cidr_parse(NULL, actualCIDR));
  TEST_ASSERT_EQUAL_UINT32(expAddr, actualCIDR->addr);
  TEST_ASSERT_EQUAL_UINT8(expPrefix, actualCIDR->prefix);
  TEST_ASSERT_FALSE(cidr_parse(cidrText, NULL));

  // Boundary checks: /0 and /32
  TEST_ASSERT_TRUE(cidr_parse("0.0.0.0/0", actualCIDR));
  TEST_ASSERT_EQUAL_UINT32(0, actualCIDR->addr);
  TEST_ASSERT_EQUAL_UINT8(0, actualCIDR->prefix);

  TEST_ASSERT_TRUE(cidr_parse("255.255.255.255/32", actualCIDR));
  TEST_ASSERT_EQUAL_UINT32(0xFFFFFFFF, actualCIDR->addr);
  TEST_ASSERT_EQUAL_UINT8(32, actualCIDR->prefix);

  free(actualCIDR);
}

void cidr_format_functionality_test(void) {
  //Initialize Data for the tests
  const char * expIPText = "192.168.1.1/24";

  const uint8_t octet1 = 192;
  const uint8_t octet2 = 168;
  const uint8_t octet3 = 1;
  const uint8_t octet4 = 1;
  const uint8_t inPrefix = 24;

  const uint32_t inAddr = ((uint32_t)octet1 << 24) |
                       ((uint32_t)octet2 << 16) |
                       ((uint32_t)octet3 << 8)  |
                       (uint32_t)octet4;

  const cidr_t inCIDR = {inAddr, inPrefix};

  char returnIP[CIDR_STR_LEN];

  //Only one case because this should be getting the output of ip_parse
  cidr_format(inCIDR, returnIP);

  TEST_ASSERT_EQUAL_STRING(expIPText, returnIP);

  // Boundary check: prefix > 32 should format as 32
  const cidr_t overPrefixCIDR = {inAddr, 35};
  cidr_format(overPrefixCIDR, returnIP);
  TEST_ASSERT_EQUAL_STRING("192.168.1.1/32", returnIP);

  // Boundary check: 0.0.0.0/0
  const cidr_t zeroCIDR = {0, 0};
  cidr_format(zeroCIDR, returnIP);
  TEST_ASSERT_EQUAL_STRING("0.0.0.0/0", returnIP);
}

//--------------------------------//
//      Task 2 Method Tests       //
//--------------------------------//

void cidr_mask_functionality_test(void) {
  // /0 must be 0 (no bits set; avoiding UB shift by 32)
  TEST_ASSERT_EQUAL_HEX32(0x00000000, cidr_mask(0));

  // Representative prefix lengths
  TEST_ASSERT_EQUAL_HEX32(0x80000000, cidr_mask(1));
  TEST_ASSERT_EQUAL_HEX32(0xFF000000, cidr_mask(8));
  TEST_ASSERT_EQUAL_HEX32(0xFFFF0000, cidr_mask(16));
  TEST_ASSERT_EQUAL_HEX32(0xFFFFFF00, cidr_mask(24));
  TEST_ASSERT_EQUAL_HEX32(0xFFFFFF80, cidr_mask(25));
  TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFC, cidr_mask(30));
  TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFE, cidr_mask(31));
  TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFF, cidr_mask(32));

  // Anything larger than 32 is treated as 32
  TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFF, cidr_mask(33));
  TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFF, cidr_mask(100));
  TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFF, cidr_mask(255));
}

void cidr_network_functionality_test(void) {
  // 132.178.227.11/25 -> 132.178.227.0
  cidr_t onyx = make_cidr(132, 178, 227, 11, 25);
  TEST_ASSERT_EQUAL_UINT32(make_ip(132, 178, 227, 0), cidr_network(onyx));

  // 192.168.1.100/24 -> 192.168.1.0
  cidr_t c24 = make_cidr(192, 168, 1, 100, 24);
  TEST_ASSERT_EQUAL_UINT32(make_ip(192, 168, 1, 0), cidr_network(c24));

  // /0 clears all host bits -> 0.0.0.0
  cidr_t zero = make_cidr(192, 168, 1, 100, 0);
  TEST_ASSERT_EQUAL_UINT32(0, cidr_network(zero));

  // /32 keeps all bits (no host bits to clear)
  cidr_t single = make_cidr(10, 20, 30, 40, 32);
  TEST_ASSERT_EQUAL_UINT32(make_ip(10, 20, 30, 40), cidr_network(single));

  // /31 point-to-point link
  cidr_t p2p = make_cidr(192, 168, 1, 5, 31);
  TEST_ASSERT_EQUAL_UINT32(make_ip(192, 168, 1, 4), cidr_network(p2p));

  // Prefix > 32 treated as 32
  cidr_t over = make_cidr(10, 20, 30, 40, 35);
  TEST_ASSERT_EQUAL_UINT32(make_ip(10, 20, 30, 40), cidr_network(over));
}

void cidr_broadcast_functionality_test(void) {
  // 132.178.227.11/25 -> 132.178.227.127
  cidr_t onyx = make_cidr(132, 178, 227, 11, 25);
  TEST_ASSERT_EQUAL_UINT32(make_ip(132, 178, 227, 127), cidr_broadcast(onyx));

  // 192.168.1.100/24 -> 192.168.1.255
  cidr_t c24 = make_cidr(192, 168, 1, 100, 24);
  TEST_ASSERT_EQUAL_UINT32(make_ip(192, 168, 1, 255), cidr_broadcast(c24));

  // /0 sets all host bits -> 255.255.255.255
  cidr_t zero = make_cidr(0, 0, 0, 0, 0);
  TEST_ASSERT_EQUAL_UINT32(0xFFFFFFFF, cidr_broadcast(zero));

  // /32 broadcast is address itself
  cidr_t single = make_cidr(10, 20, 30, 40, 32);
  TEST_ASSERT_EQUAL_UINT32(make_ip(10, 20, 30, 40), cidr_broadcast(single));

  // /31 point-to-point
  cidr_t p2p = make_cidr(192, 168, 1, 4, 31);
  TEST_ASSERT_EQUAL_UINT32(make_ip(192, 168, 1, 5), cidr_broadcast(p2p));

  // Prefix > 32 treated as 32
  cidr_t over = make_cidr(10, 20, 30, 40, 35);
  TEST_ASSERT_EQUAL_UINT32(make_ip(10, 20, 30, 40), cidr_broadcast(over));
}

void cidr_first_host_functionality_test(void) {
  // /0 to /30: network + 1
  cidr_t onyx = make_cidr(132, 178, 227, 11, 25);
  TEST_ASSERT_EQUAL_UINT32(make_ip(132, 178, 227, 1), cidr_first_host(onyx));

  cidr_t c24 = make_cidr(192, 168, 1, 0, 24);
  TEST_ASSERT_EQUAL_UINT32(make_ip(192, 168, 1, 1), cidr_first_host(c24));

  cidr_t c30 = make_cidr(10, 0, 0, 4, 30);
  TEST_ASSERT_EQUAL_UINT32(make_ip(10, 0, 0, 5), cidr_first_host(c30));

  cidr_t zero = make_cidr(0, 0, 0, 0, 0);
  TEST_ASSERT_EQUAL_UINT32(1, cidr_first_host(zero)); // 0.0.0.1

  // /31: point-to-point (RFC 3021), first host is the network address
  cidr_t c31 = make_cidr(192, 168, 1, 5, 31); // network is .4
  TEST_ASSERT_EQUAL_UINT32(make_ip(192, 168, 1, 4), cidr_first_host(c31));

  // /32: single host, first host is the network address (the address itself)
  cidr_t c32 = make_cidr(192, 168, 1, 50, 32);
  TEST_ASSERT_EQUAL_UINT32(make_ip(192, 168, 1, 50), cidr_first_host(c32));

  // Prefix > 32: treated as 32
  cidr_t over = make_cidr(192, 168, 1, 50, 35);
  TEST_ASSERT_EQUAL_UINT32(make_ip(192, 168, 1, 50), cidr_first_host(over));
}

void cidr_last_host_functionality_test(void) {
  // /0 to /30: broadcast - 1
  cidr_t onyx = make_cidr(132, 178, 227, 11, 25);
  TEST_ASSERT_EQUAL_UINT32(make_ip(132, 178, 227, 126), cidr_last_host(onyx));

  cidr_t c24 = make_cidr(192, 168, 1, 0, 24);
  TEST_ASSERT_EQUAL_UINT32(make_ip(192, 168, 1, 254), cidr_last_host(c24));

  cidr_t c30 = make_cidr(10, 0, 0, 4, 30); // broadcast is .7
  TEST_ASSERT_EQUAL_UINT32(make_ip(10, 0, 0, 6), cidr_last_host(c30));

  cidr_t zero = make_cidr(0, 0, 0, 0, 0);
  TEST_ASSERT_EQUAL_UINT32(0xFFFFFFFE, cidr_last_host(zero)); // 255.255.255.254

  // /31: broadcast address itself
  cidr_t c31 = make_cidr(192, 168, 1, 4, 31); // broadcast is .5
  TEST_ASSERT_EQUAL_UINT32(make_ip(192, 168, 1, 5), cidr_last_host(c31));

  // /32: broadcast address itself (the address itself)
  cidr_t c32 = make_cidr(192, 168, 1, 50, 32);
  TEST_ASSERT_EQUAL_UINT32(make_ip(192, 168, 1, 50), cidr_last_host(c32));

  // Prefix > 32: treated as 32
  cidr_t over = make_cidr(192, 168, 1, 50, 35);
  TEST_ASSERT_EQUAL_UINT32(make_ip(192, 168, 1, 50), cidr_last_host(over));
}

void cidr_host_count_functionality_test(void) {
  // /25: 126
  cidr_t onyx = make_cidr(132, 178, 227, 11, 25);
  TEST_ASSERT_EQUAL_UINT64(126ULL, cidr_host_count(onyx));

  // /24: 254
  cidr_t c24 = make_cidr(192, 168, 1, 0, 24);
  TEST_ASSERT_EQUAL_UINT64(254ULL, cidr_host_count(c24));

  // /16: 65534
  cidr_t c16 = make_cidr(10, 0, 0, 0, 16);
  TEST_ASSERT_EQUAL_UINT64(65534ULL, cidr_host_count(c16));

  // /30: 2
  cidr_t c30 = make_cidr(10, 0, 0, 0, 30);
  TEST_ASSERT_EQUAL_UINT64(2ULL, cidr_host_count(c30));

  // /31: 2 (RFC 3021)
  cidr_t c31 = make_cidr(10, 0, 0, 0, 31);
  TEST_ASSERT_EQUAL_UINT64(2ULL, cidr_host_count(c31));

  // /32: 1
  cidr_t c32 = make_cidr(10, 0, 0, 0, 32);
  TEST_ASSERT_EQUAL_UINT64(1ULL, cidr_host_count(c32));

  // /0: 2^32 - 2 = 4294967294 (requires 64 bits)
  cidr_t zero = make_cidr(0, 0, 0, 0, 0);
  TEST_ASSERT_EQUAL_UINT64(4294967294ULL, cidr_host_count(zero));

  // Prefix > 32: treated as 32 -> 1
  cidr_t over = make_cidr(10, 0, 0, 0, 35);
  TEST_ASSERT_EQUAL_UINT64(1ULL, cidr_host_count(over));
}

void cidr_contains_functionality_test(void) {
  // For 132.178.227.11/25 (from Onyx assignment description):
  // .50 yes, .200 no
  cidr_t onyx = make_cidr(132, 178, 227, 11, 25);
  TEST_ASSERT_TRUE(cidr_contains(onyx, make_ip(132, 178, 227, 0)));    // network
  TEST_ASSERT_TRUE(cidr_contains(onyx, make_ip(132, 178, 227, 50)));   // host inside
  TEST_ASSERT_TRUE(cidr_contains(onyx, make_ip(132, 178, 227, 127)));  // broadcast
  TEST_ASSERT_FALSE(cidr_contains(onyx, make_ip(132, 178, 227, 128))); // outside (next subnet)
  TEST_ASSERT_FALSE(cidr_contains(onyx, make_ip(132, 178, 227, 200))); // outside
  TEST_ASSERT_FALSE(cidr_contains(onyx, make_ip(10, 0, 0, 1)));        // different network

  // /0 contains everything
  cidr_t zero = make_cidr(0, 0, 0, 0, 0);
  TEST_ASSERT_TRUE(cidr_contains(zero, 0));
  TEST_ASSERT_TRUE(cidr_contains(zero, make_ip(192, 168, 1, 1)));
  TEST_ASSERT_TRUE(cidr_contains(zero, 0xFFFFFFFF));

  // /32 contains only that exact address
  cidr_t single = make_cidr(192, 168, 1, 10, 32);
  TEST_ASSERT_TRUE(cidr_contains(single, make_ip(192, 168, 1, 10)));
  TEST_ASSERT_FALSE(cidr_contains(single, make_ip(192, 168, 1, 9)));
  TEST_ASSERT_FALSE(cidr_contains(single, make_ip(192, 168, 1, 11)));

  // Prefix > 32 treated as 32
  cidr_t over = make_cidr(192, 168, 1, 10, 35);
  TEST_ASSERT_TRUE(cidr_contains(over, make_ip(192, 168, 1, 10)));
  TEST_ASSERT_FALSE(cidr_contains(over, make_ip(192, 168, 1, 11)));
}

void cidr_is_network_functionality_test(void) {
  // Host bits all zero -> true
  TEST_ASSERT_TRUE(cidr_is_network(make_cidr(192, 168, 1, 0, 24)));
  TEST_ASSERT_TRUE(cidr_is_network(make_cidr(132, 178, 227, 0, 25)));
  TEST_ASSERT_TRUE(cidr_is_network(make_cidr(132, 178, 227, 128, 25)));
  TEST_ASSERT_TRUE(cidr_is_network(make_cidr(0, 0, 0, 0, 0)));
  TEST_ASSERT_TRUE(cidr_is_network(make_cidr(10, 0, 0, 0, 8)));

  // Host bits not all zero -> false
  TEST_ASSERT_FALSE(cidr_is_network(make_cidr(192, 168, 1, 1, 24)));
  TEST_ASSERT_FALSE(cidr_is_network(make_cidr(192, 168, 1, 5, 24)));
  TEST_ASSERT_FALSE(cidr_is_network(make_cidr(132, 178, 227, 11, 25)));
  TEST_ASSERT_FALSE(cidr_is_network(make_cidr(1, 2, 3, 4, 0))); // for /0, all 32 bits are host bits!

  // /31: even addresses have lowest bit 0 -> network; odd addresses have bit 1 -> host
  TEST_ASSERT_TRUE(cidr_is_network(make_cidr(192, 168, 1, 4, 31)));
  TEST_ASSERT_FALSE(cidr_is_network(make_cidr(192, 168, 1, 5, 31)));

  // /32: 0 host bits, host bits are all zero -> true
  TEST_ASSERT_TRUE(cidr_is_network(make_cidr(192, 168, 1, 1, 32)));

  // Prefix > 32 treated as 32
  TEST_ASSERT_TRUE(cidr_is_network(make_cidr(192, 168, 1, 1, 40)));
}

void cidr_subnet_functionality_test(void) {
  cidr_t out = {0, 0};

  // Valid splitting: 192.168.1.0/24 into 4 subnets (all /26)
  cidr_t base24 = make_cidr(192, 168, 1, 0, 24);

  TEST_ASSERT_TRUE(cidr_subnet(base24, 4, 0, &out));
  TEST_ASSERT_EQUAL_UINT32(make_ip(192, 168, 1, 0), out.addr);
  TEST_ASSERT_EQUAL_UINT8(26, out.prefix);

  TEST_ASSERT_TRUE(cidr_subnet(base24, 4, 1, &out));
  TEST_ASSERT_EQUAL_UINT32(make_ip(192, 168, 1, 64), out.addr);
  TEST_ASSERT_EQUAL_UINT8(26, out.prefix);

  TEST_ASSERT_TRUE(cidr_subnet(base24, 4, 2, &out));
  TEST_ASSERT_EQUAL_UINT32(make_ip(192, 168, 1, 128), out.addr);
  TEST_ASSERT_EQUAL_UINT8(26, out.prefix);

  TEST_ASSERT_TRUE(cidr_subnet(base24, 4, 3, &out));
  TEST_ASSERT_EQUAL_UINT32(make_ip(192, 168, 1, 192), out.addr);
  TEST_ASSERT_EQUAL_UINT8(26, out.prefix);

  // Splitting into 1 subnet (n = 1): returns the block unchanged
  TEST_ASSERT_TRUE(cidr_subnet(base24, 1, 0, &out));
  TEST_ASSERT_EQUAL_UINT32(base24.addr, out.addr);
  TEST_ASSERT_EQUAL_UINT8(base24.prefix, out.prefix);

  // Splitting /0 into 2 subnets: 0.0.0.0/1 and 128.0.0.0/1
  cidr_t base0 = make_cidr(0, 0, 0, 0, 0);
  TEST_ASSERT_TRUE(cidr_subnet(base0, 2, 0, &out));
  TEST_ASSERT_EQUAL_UINT32(make_ip(0, 0, 0, 0), out.addr);
  TEST_ASSERT_EQUAL_UINT8(1, out.prefix);

  TEST_ASSERT_TRUE(cidr_subnet(base0, 2, 1, &out));
  TEST_ASSERT_EQUAL_UINT32(make_ip(128, 0, 0, 0), out.addr);
  TEST_ASSERT_EQUAL_UINT8(1, out.prefix);

  // Failure cases: out must remain untouched on failure!
  cidr_t sentinel = make_cidr(99, 99, 99, 99, 99);
  out = sentinel;

  // 1. out is NULL
  TEST_ASSERT_FALSE(cidr_subnet(base24, 4, 0, NULL));

  // 2. Block is not a network (host bits set)
  cidr_t notNet = make_cidr(192, 168, 1, 5, 24);
  TEST_ASSERT_FALSE(cidr_subnet(notNet, 4, 0, &out));
  TEST_ASSERT_EQUAL_UINT32(sentinel.addr, out.addr);
  TEST_ASSERT_EQUAL_UINT8(sentinel.prefix, out.prefix);

  // 3. n is 0 or not a power of two
  TEST_ASSERT_FALSE(cidr_subnet(base24, 0, 0, &out));
  TEST_ASSERT_EQUAL_UINT32(sentinel.addr, out.addr);
  TEST_ASSERT_FALSE(cidr_subnet(base24, 3, 0, &out));
  TEST_ASSERT_EQUAL_UINT32(sentinel.addr, out.addr);
  TEST_ASSERT_FALSE(cidr_subnet(base24, 5, 0, &out));
  TEST_ASSERT_EQUAL_UINT32(sentinel.addr, out.addr);
  TEST_ASSERT_FALSE(cidr_subnet(base24, 6, 0, &out));
  TEST_ASSERT_EQUAL_UINT32(sentinel.addr, out.addr);

  // 4. i is out of range (i >= n)
  TEST_ASSERT_FALSE(cidr_subnet(base24, 4, 4, &out));
  TEST_ASSERT_EQUAL_UINT32(sentinel.addr, out.addr);
  TEST_ASSERT_FALSE(cidr_subnet(base24, 4, 5, &out));
  TEST_ASSERT_EQUAL_UINT32(sentinel.addr, out.addr);

  // 5. Split would need a prefix longer than 32
  // /32 cannot be split (even n = 2 would need /33)
  cidr_t base32 = make_cidr(192, 168, 1, 1, 32);
  TEST_ASSERT_FALSE(cidr_subnet(base32, 2, 0, &out));
  TEST_ASSERT_EQUAL_UINT32(sentinel.addr, out.addr);

  // /30 split into 8 subnets borrows 3 bits -> prefix 33 > 32
  cidr_t base30 = make_cidr(192, 168, 1, 0, 30);
  TEST_ASSERT_FALSE(cidr_subnet(base30, 8, 0, &out));
  TEST_ASSERT_EQUAL_UINT32(sentinel.addr, out.addr);
}

//--------------------------------//
//      Task 3 Method Tests       //
//--------------------------------//

void route_parse_functionality_test(void) {
  route_t r;
  memset(&r, 0, sizeof(r));

  // Valid route: regular CIDR with dotted IP gateway
  TEST_ASSERT_TRUE(route_parse("10.0.0.0/8 10.1.0.1 eth1", &r));
  TEST_ASSERT_EQUAL_UINT32(make_ip(10, 0, 0, 0), r.dest.addr);
  TEST_ASSERT_EQUAL_UINT8(8, r.dest.prefix);
  TEST_ASSERT_EQUAL_UINT32(make_ip(10, 1, 0, 1), r.gateway);
  TEST_ASSERT_EQUAL_STRING("eth1", r.dev);

  // Valid route: "default" destination -> 0.0.0.0/0
  TEST_ASSERT_TRUE(route_parse("default 132.178.227.1 eno1", &r));
  TEST_ASSERT_EQUAL_UINT32(0, r.dest.addr);
  TEST_ASSERT_EQUAL_UINT8(0, r.dest.prefix);
  TEST_ASSERT_EQUAL_UINT32(make_ip(132, 178, 227, 1), r.gateway);
  TEST_ASSERT_EQUAL_STRING("eno1", r.dev);

  // Valid route: "direct" gateway -> 0
  TEST_ASSERT_TRUE(route_parse("132.178.227.0/25 direct eno1", &r));
  TEST_ASSERT_EQUAL_UINT32(make_ip(132, 178, 227, 0), r.dest.addr);
  TEST_ASSERT_EQUAL_UINT8(25, r.dest.prefix);
  TEST_ASSERT_EQUAL_UINT32(0, r.gateway);
  TEST_ASSERT_EQUAL_STRING("eno1", r.dev);

  // Valid route: "default" with "direct"
  TEST_ASSERT_TRUE(route_parse("default direct eth0", &r));
  TEST_ASSERT_EQUAL_UINT32(0, r.dest.addr);
  TEST_ASSERT_EQUAL_UINT8(0, r.dest.prefix);
  TEST_ASSERT_EQUAL_UINT32(0, r.gateway);
  TEST_ASSERT_EQUAL_STRING("eth0", r.dev);

  // Valid route: leading/trailing whitespace, multiple spaces and tabs, trailing newline
  TEST_ASSERT_TRUE(route_parse(" \t 192.168.0.0/16 \t\t 10.1.0.1 \t eth1 \r\n", &r));
  TEST_ASSERT_EQUAL_UINT32(make_ip(192, 168, 0, 0), r.dest.addr);
  TEST_ASSERT_EQUAL_UINT8(16, r.dest.prefix);
  TEST_ASSERT_EQUAL_UINT32(make_ip(10, 1, 0, 1), r.gateway);
  TEST_ASSERT_EQUAL_STRING("eth1", r.dev);

  // Valid route: maximum device name length (DEV_NAME_LEN - 1 = 15 chars)
  TEST_ASSERT_TRUE(route_parse("10.0.0.0/8 direct 123456789012345", &r));
  TEST_ASSERT_EQUAL_STRING("123456789012345", r.dev);

  // Failure cases: r must remain untouched on failure!
  route_t sentinel = make_route(make_cidr(99, 99, 99, 99, 99), 0x12345678, "sentinel");
  r = sentinel;

  // 1. NULL arguments
  TEST_ASSERT_FALSE(route_parse(NULL, &r));
  TEST_ASSERT_EQUAL_UINT32(sentinel.dest.addr, r.dest.addr);
  TEST_ASSERT_FALSE(route_parse("default direct eth0", NULL));

  // 2. Destination has host bits set
  TEST_ASSERT_FALSE(route_parse("192.168.1.5/24 direct eth0", &r));
  TEST_ASSERT_EQUAL_UINT32(sentinel.dest.addr, r.dest.addr);
  TEST_ASSERT_FALSE(route_parse("132.178.227.11/25 10.1.0.1 eth0", &r));
  TEST_ASSERT_EQUAL_UINT32(sentinel.dest.addr, r.dest.addr);

  // 3. Malformed destination
  TEST_ASSERT_FALSE(route_parse("10.0.0/8 direct eth0", &r));
  TEST_ASSERT_FALSE(route_parse("10.0.0.0/33 direct eth0", &r));
  TEST_ASSERT_FALSE(route_parse("10.0.0.0 direct eth0", &r));
  TEST_ASSERT_FALSE(route_parse("defaul direct eth0", &r));

  // 4. Malformed gateway
  TEST_ASSERT_FALSE(route_parse("10.0.0.0/8 10.1.0 eth1", &r));
  TEST_ASSERT_FALSE(route_parse("10.0.0.0/8 256.1.1.1 eth1", &r));
  TEST_ASSERT_FALSE(route_parse("10.0.0.0/8 010.0.0.1 eth1", &r));
  TEST_ASSERT_FALSE(route_parse("10.0.0.0/8 indirectly eth1", &r));

  // 5. Invalid device: empty or too long (>= DEV_NAME_LEN chars)
  TEST_ASSERT_FALSE(route_parse("10.0.0.0/8 direct", &r));
  TEST_ASSERT_FALSE(route_parse("10.0.0.0/8 direct 1234567890123456", &r));

  // 6. Extra fields / comments not stripped by route_parse
  TEST_ASSERT_FALSE(route_parse("default direct eth0 extra", &r));
  TEST_ASSERT_FALSE(route_parse("default direct eth0 # comment", &r));
  TEST_ASSERT_FALSE(route_parse("", &r));
}

void table_create_functionality_test(void) {
  fwd_table_t *table = table_create();
  TEST_ASSERT_NOT_NULL(table);
  TEST_ASSERT_EQUAL_size_t(0, table_count(table));
  table_destroy(table);
}

void table_destroy_functionality_test(void) {
  // NULL should do nothing and not crash
  table_destroy(NULL);

  // Empty table destroy
  fwd_table_t *table = table_create();
  TEST_ASSERT_NOT_NULL(table);
  table_destroy(table);

  // Populated table destroy
  table = table_create();
  route_t r = make_route(make_cidr(10, 0, 0, 0, 8), make_ip(10, 1, 0, 1), "eth1");
  table_add(table, &r);
  table_destroy(table);
}

void table_count_functionality_test(void) {
  // NULL table returns 0
  TEST_ASSERT_EQUAL_size_t(0, table_count(NULL));

  fwd_table_t *table = table_create();
  TEST_ASSERT_EQUAL_size_t(0, table_count(table));

  route_t r1 = make_route(make_cidr(0, 0, 0, 0, 0), make_ip(132, 178, 227, 1), "eno1");
  TEST_ASSERT_TRUE(table_add(table, &r1));
  TEST_ASSERT_EQUAL_size_t(1, table_count(table));

  route_t r2 = make_route(make_cidr(10, 0, 0, 0, 8), make_ip(10, 1, 0, 1), "eth1");
  TEST_ASSERT_TRUE(table_add(table, &r2));
  TEST_ASSERT_EQUAL_size_t(2, table_count(table));

  // Failed add does not increment count
  TEST_ASSERT_FALSE(table_add(table, &r2)); // duplicate
  TEST_ASSERT_EQUAL_size_t(2, table_count(table));

  table_destroy(table);
}

void table_add_functionality_test(void) {
  fwd_table_t *table = table_create();
  TEST_ASSERT_NOT_NULL(table);

  // Add valid route
  route_t r1 = make_route(make_cidr(10, 0, 0, 0, 8), make_ip(10, 1, 0, 1), "eth1");
  TEST_ASSERT_TRUE(table_add(table, &r1));
  TEST_ASSERT_EQUAL_size_t(1, table_count(table));

  // Add another route
  route_t r2 = make_route(make_cidr(10, 1, 0, 0, 16), 0, "eth1");
  TEST_ASSERT_TRUE(table_add(table, &r2));
  TEST_ASSERT_EQUAL_size_t(2, table_count(table));

  // Add default route
  route_t rdef = make_route(make_cidr(0, 0, 0, 0, 0), make_ip(132, 178, 227, 1), "eno1");
  TEST_ASSERT_TRUE(table_add(table, &rdef));
  TEST_ASSERT_EQUAL_size_t(3, table_count(table));

  // Failure: duplicate destination block (even with different gateway or device)
  route_t r_dup = make_route(make_cidr(10, 0, 0, 0, 8), make_ip(10, 2, 0, 1), "eth2");
  TEST_ASSERT_FALSE(table_add(table, &r_dup));
  TEST_ASSERT_EQUAL_size_t(3, table_count(table));

  // Failure: duplicate default route
  route_t rdef_dup = make_route(make_cidr(0, 0, 0, 0, 0), 0, "eth0");
  TEST_ASSERT_FALSE(table_add(table, &rdef_dup));
  TEST_ASSERT_EQUAL_size_t(3, table_count(table));

  // Failure: destination has host bits set
  route_t r_hostbits = make_route(make_cidr(192, 168, 1, 5, 24), 0, "eth0");
  TEST_ASSERT_FALSE(table_add(table, &r_hostbits));
  TEST_ASSERT_EQUAL_size_t(3, table_count(table));

  // Failure: NULL arguments
  TEST_ASSERT_FALSE(table_add(NULL, &r1));
  TEST_ASSERT_FALSE(table_add(table, NULL));
  TEST_ASSERT_EQUAL_size_t(3, table_count(table));

  table_destroy(table);
}

void table_load_functionality_test(void) {
  fwd_table_t *table = table_create();
  size_t bad_line = 999;

  // Failure: NULL arguments
  TEST_ASSERT_FALSE(table_load(NULL, NULL, &bad_line));
  TEST_ASSERT_EQUAL_size_t(0, bad_line);

  TEST_ASSERT_FALSE(table_load(table, NULL, &bad_line));
  TEST_ASSERT_EQUAL_size_t(0, bad_line);

  // Valid table file loading (Canvas Assignment example)
  const char *valid_content =
    "# destination      gateway          device\n"
    "default            132.178.227.1    eno1\n"
    "132.178.227.0/25   direct           eno1\n"
    "\n"
    "10.0.0.0/8         10.1.0.1         eth1\n"
    "10.1.0.0/16        direct           eth1\n"
    "10.1.2.0/24        direct           eth2\n"
    "10.1.2.128/25      10.1.2.1         eth2   # a router behind eth2\n"
    "192.168.0.0/16     10.1.0.1         eth1\n";

  FILE *f = create_temp_file(valid_content);
  TEST_ASSERT_NOT_NULL(f);
  bad_line = 999;
  TEST_ASSERT_TRUE(table_load(table, f, &bad_line));
  TEST_ASSERT_EQUAL_size_t(7, table_count(table));
  fclose(f);
  table_destroy(table);

  // File with an error on line 4
  const char *bad_content =
    "# Line 1: comment\n"
    "default 132.178.227.1 eno1\n"
    "10.0.0.0/8 direct eth1\n"
    "192.168.1.5/24 direct eth2\n" // Bad line 4: host bits set
    "10.1.0.0/16 direct eth1\n";

  table = table_create();
  f = create_temp_file(bad_content);
  TEST_ASSERT_NOT_NULL(f);
  bad_line = 0;
  TEST_ASSERT_FALSE(table_load(table, f, &bad_line));
  TEST_ASSERT_EQUAL_size_t(4, bad_line);
  // Routes before line 4 stay in the table!
  TEST_ASSERT_EQUAL_size_t(2, table_count(table));
  fclose(f);
  table_destroy(table);

  // bad_line parameter is optional (can be NULL)
  table = table_create();
  f = create_temp_file(bad_content);
  TEST_ASSERT_NOT_NULL(f);
  TEST_ASSERT_FALSE(table_load(table, f, NULL));
  fclose(f);
  table_destroy(table);
}

void table_lookup_functionality_test(void) {
  // NULL table returns NULL
  TEST_ASSERT_NULL(table_lookup(NULL, make_ip(10, 1, 2, 5)));

  // Empty table returns NULL
  fwd_table_t *table = table_create();
  TEST_ASSERT_NULL(table_lookup(table, make_ip(10, 1, 2, 5)));

  // Table without default route: unmatched address returns NULL
  route_t r_ten = make_route(make_cidr(10, 0, 0, 0, 8), make_ip(10, 1, 0, 1), "eth1");
  table_add(table, &r_ten);
  TEST_ASSERT_NULL(table_lookup(table, make_ip(192, 168, 1, 1)));

  // Match inside single route
  const route_t *match = table_lookup(table, make_ip(10, 5, 5, 5));
  TEST_ASSERT_NOT_NULL(match);
  TEST_ASSERT_EQUAL_UINT32(make_ip(10, 0, 0, 0), match->dest.addr);
  TEST_ASSERT_EQUAL_STRING("eth1", match->dev);
  table_destroy(table);

  // Longest Prefix Match test (Canvas Assignment Onyx example)
  // Routes:
  // default            132.178.227.1    eno1
  // 132.178.227.0/25   direct           eno1
  // 10.0.0.0/8         10.1.0.1         eth1
  // 10.1.0.0/16        direct           eth1
  // 10.1.2.0/24        direct           eth2
  // 10.1.2.128/25      10.1.2.1         eth2
  // 192.168.0.0/16     10.1.0.1         eth1
  table = table_create();
  route_t routes[7] = {
    make_route(make_cidr(0, 0, 0, 0, 0), make_ip(132, 178, 227, 1), "eno1"),
    make_route(make_cidr(132, 178, 227, 0, 25), 0, "eno1"),
    make_route(make_cidr(10, 0, 0, 0, 8), make_ip(10, 1, 0, 1), "eth1"),
    make_route(make_cidr(10, 1, 0, 0, 16), 0, "eth1"),
    make_route(make_cidr(10, 1, 2, 0, 24), 0, "eth2"),
    make_route(make_cidr(10, 1, 2, 128, 25), make_ip(10, 1, 2, 1), "eth2"),
    make_route(make_cidr(192, 168, 0, 0, 16), make_ip(10, 1, 0, 1), "eth1")
  };
  for (int i = 0; i < 7; i++) {
    table_add(table, &routes[i]);
  }

  // 132.178.227.50 -> 132.178.227.0/25 dev eno1 direct
  match = table_lookup(table, make_ip(132, 178, 227, 50));
  TEST_ASSERT_NOT_NULL(match);
  TEST_ASSERT_EQUAL_UINT32(make_ip(132, 178, 227, 0), match->dest.addr);
  TEST_ASSERT_EQUAL_UINT8(25, match->dest.prefix);
  TEST_ASSERT_EQUAL_UINT32(0, match->gateway);
  TEST_ASSERT_EQUAL_STRING("eno1", match->dev);

  // 8.8.8.8 -> default (0.0.0.0/0) via 132.178.227.1 dev eno1
  match = table_lookup(table, make_ip(8, 8, 8, 8));
  TEST_ASSERT_NOT_NULL(match);
  TEST_ASSERT_EQUAL_UINT32(0, match->dest.addr);
  TEST_ASSERT_EQUAL_UINT8(0, match->dest.prefix);
  TEST_ASSERT_EQUAL_UINT32(make_ip(132, 178, 227, 1), match->gateway);
  TEST_ASSERT_EQUAL_STRING("eno1", match->dev);

  // 10.1.2.5 -> 10.1.2.0/24 dev eth2 direct
  match = table_lookup(table, make_ip(10, 1, 2, 5));
  TEST_ASSERT_NOT_NULL(match);
  TEST_ASSERT_EQUAL_UINT32(make_ip(10, 1, 2, 0), match->dest.addr);
  TEST_ASSERT_EQUAL_UINT8(24, match->dest.prefix);
  TEST_ASSERT_EQUAL_UINT32(0, match->gateway);
  TEST_ASSERT_EQUAL_STRING("eth2", match->dev);

  // 10.1.2.200 -> 10.1.2.128/25 via 10.1.2.1 dev eth2
  match = table_lookup(table, make_ip(10, 1, 2, 200));
  TEST_ASSERT_NOT_NULL(match);
  TEST_ASSERT_EQUAL_UINT32(make_ip(10, 1, 2, 128), match->dest.addr);
  TEST_ASSERT_EQUAL_UINT8(25, match->dest.prefix);
  TEST_ASSERT_EQUAL_UINT32(make_ip(10, 1, 2, 1), match->gateway);
  TEST_ASSERT_EQUAL_STRING("eth2", match->dev);

  table_destroy(table);

  // Order independence test: add in reverse order
  fwd_table_t *rev_table = table_create();
  for (int i = 6; i >= 0; i--) {
    table_add(rev_table, &routes[i]);
  }
  // Lookup 10.1.2.200 must still return /25 route
  match = table_lookup(rev_table, make_ip(10, 1, 2, 200));
  TEST_ASSERT_NOT_NULL(match);
  TEST_ASSERT_EQUAL_UINT8(25, match->dest.prefix);
  TEST_ASSERT_EQUAL_UINT32(make_ip(10, 1, 2, 128), match->dest.addr);
  TEST_ASSERT_EQUAL_STRING("eth2", match->dev);

  table_destroy(rev_table);
}

//--------------------------------//
//      Test Runner Main          //
//--------------------------------//

int main(void) {
  UNITY_BEGIN();
  printf(BLUE "\n\nRunning Tests for Project 3 CS425\n" RESET);
  printf(YELLOW "-----------------------------------\n" RESET);
  printf(CYAN "Running Task 1 Method tests...\n" RESET);
  printf(YELLOW "-----------------------------------\n" RESET);
  RUN_TEST(ip_parse_functionality_test);
  RUN_TEST(ip_format_functionality_test);
  RUN_TEST(cidr_parse_functionality_test);
  RUN_TEST(cidr_format_functionality_test);
  printf(YELLOW "-----------------------------------\n" RESET);
  printf(CYAN "Running Task 2 Method tests...\n" RESET);
  printf(YELLOW "-----------------------------------\n" RESET);
  RUN_TEST(cidr_mask_functionality_test);
  RUN_TEST(cidr_network_functionality_test);
  RUN_TEST(cidr_broadcast_functionality_test);
  RUN_TEST(cidr_first_host_functionality_test);
  RUN_TEST(cidr_last_host_functionality_test);
  RUN_TEST(cidr_host_count_functionality_test);
  RUN_TEST(cidr_contains_functionality_test);
  RUN_TEST(cidr_is_network_functionality_test);
  RUN_TEST(cidr_subnet_functionality_test);
  printf(YELLOW "-----------------------------------\n" RESET);
  printf(CYAN "Running Task 3 Method tests...\n" RESET);
  printf(YELLOW "-----------------------------------\n" RESET);
  RUN_TEST(route_parse_functionality_test);
  RUN_TEST(table_create_functionality_test);
  RUN_TEST(table_destroy_functionality_test);
  RUN_TEST(table_count_functionality_test);
  RUN_TEST(table_add_functionality_test);
  RUN_TEST(table_load_functionality_test);
  RUN_TEST(table_lookup_functionality_test);
  printf(YELLOW "-----------------------------------\n" RESET);
  printf(BLUE "Tests Complete.\n" RESET);
  return UNITY_END();
}
