#include <stdlib.h>
#include <stdio.h>
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

void cidr_mask_test(void) {
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

}

void table_create_functionality_test(void) {

}

void table_destroy_functionality_test(void) {

}

void table_count_functionality_test(void) {

}

void table_add_functionality_test(void) {

}

void table_load_functionality_test(void) {

}

void table_lookup_functionality_test(void) {

}

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
  RUN_TEST(cidr_mask_test);
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
