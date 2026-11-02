# Project 3

- Name: Zachary Johnston
- Email: zacharyjohnston324@u.boisestate.edu
- Class: C425-001

## Known Bugs or Issues

No bugs known to me at submission time.

## Design

### 1. /0 (Default Route / Entire IPv4 Address Space)

- Netmask Avoidance of Undefined Behavior (cidr_mask):
  
  - In C, bit-shifting a 32-bit integer by 32 bits (i.e. 0xFFFFFFFFU << (32 - 0)) is Undefined Behavior (UB) per C standard 6.5.7.
  - Handled with an explicit zero guard:

        if (prefix == 0) {
          return 0; // 0.0.0.0
        }

- Network & Broadcast Addresses (cidr_network, cidr_broadcast):
  
  - Network address: addr & 0 = 0.0.0.0.
  - Broadcast address: addr | ~0 = 255.255.255.255.

- 64-bit Host Count (cidr_host_count):
  
  - The standard host formula is 2^{32-prefix} - 2. For /0, 2³² = 4, 294, 967, 296, which overflows a standard 32-bit uint32_t (UINT32_MAX is 4, 294, 967, 295).
  - Evaluated using 64-bit arithmetic:

        return (1ULL << (32 - block.prefix)) - 2; // Returns 4,294,967,294

- First & Last Host (cidr_first_host, cidr_last_host):
  
  - First host: 0.0.0.1 (network + 1).
  - Last host: 255.255.255.254 (broadcast - 1).

- Routing & Containment (cidr_contains, table_lookup):

  - Evaluates (addr & 0) == (0 & 0), matching all IPv4 addresses. Because its prefix length is 0, any route with a prefix > 0 will win over it during longest prefix matching.

### 2. /31 (Point-to-Point Links — RFC 3021)

- Netmask:

  - cidr_mask(31) returns 0xFFFFFFFE (255.255.255.254).

- RFC 3021 Special Host Semantics:

  - Traditional subnetting reserves 2 addresses (network and broadcast), which would leave 2 - 2 = 0 hosts. RFC 3021 defines point-to-point links where both addresses are assigned to the two router interfaces:

    - First Host: The network address itself (net).
    - Last Host: The broadcast address itself (bcast).

          uint32_t cidr_first_host(cidr_t block) {
            uint32_t net = cidr_network(block);
            if (block.prefix >= 31) {
              return net;
            }
            return net + 1;
          }

          uint32_t cidr_last_host(cidr_t block) {
            uint32_t bcast = cidr_broadcast(block);
            if (block.prefix >= 31) {
              return bcast;
            }
            return bcast - 1;
          }

    - Host Count: Exactly 2 hosts:

          if (block.prefix == 31) {
            return 2;
          }

### 3. /32 (Single Host / Loopback)

- Netmask:

  - cidr_mask(32) returns 0xFFFFFFFF (255.255.255.255). Any prefix > 32 is clamped to 32.

- Single-Host Semantics:

  - Network and broadcast addresses are identical to the host address itself.
  - First Host & Last Host: Equal to the address itself (shares the prefix >= 31 check with /31). -1 Host Count: Exactly 1 host:

        if (block.prefix >= 32) {
          return 1;
        }

- Lookup:

  - Matches only the exact destination IP. In table_lookup, prefix 32 is the maximum possible prefix length and always wins when matched.

## Experience

I would've liked to write the design portion myself but I am low on time. I really need my life outside of college to stop interfering. I would've done the assignment yesterday if I didn't get stuck in a situation that left me unable to work on the project. Sorry for the sort section here. I made a few changes to hopefully get the docx working, any other changes after the due date I feel would be dishonest.

## AI Fixes Note

Note after lab 3 test implemented I reviewed what AI generated for the tests and I noticed it using memset(r.dev, 0, sizeof(r.dev) - 1) which doesn't entirely make sense for memory allocation. But at no point is the memory for the route struct allocated with a malloc in the make_route helper. This means the memory is allocated on the stack for the function which ends up being cleared when the helper function returns the memory is no longer guarenteed to exist. I changed the tester to instead using malloc and free to manage that memory.

Reference Diff in Fixing Memory Alloc in Tester for changes.
