# Instructions from Canvas

## Background

An IPv4 address is just a 32-bit number. 192.168.1.10 is the four bytes 192, 168, 1 and 10, which as one number is 0xC0A8010A. Writing it as a dotted quad is only for people.

A block in CIDR notation, like 192.168.1.0/24, is an address plus a prefix length. The prefix says how many of the leading bits name the network, and the rest name a host. So a /24 has 8 host bits and 256 addresses, and its subnet mask is 24 ones followed by 8 zeros, 255.255.255.0. Everything else is bit operations on that mask:

| Value        | How to get it                | For 132.178.227.11/25                       |
|--------------|------------------------------|---------------------------------------------|
| network      | addr & mask                  | 132.178.227.0                               |
| broadcast    | addr \| ~mask                | 132.178.227.127                             |
| usable hosts | network + 1 to broadcast - 1 | 132.178.227.1 to 132.178.227.126, 126 hosts |
| contains x?  | (x & mask) == (addr & mask)  | .50 yes, .200 no                            |

Those are the numbers you found for Onyx in A4. Two prefixes break the pattern. A /31 is a point to point link between two routers (RFC 3021), so both of its addresses are hosts and neither is a network or broadcast address. A /32 is a single host.

A forwarding table is a list of blocks, each with a gateway and an outgoing interface. When a datagram arrives, every block that contains its destination is a candidate, and the router picks the one with the longest prefix, because that is the most specific. The default route is 0.0.0.0/0, which contains every address and therefore loses to anything more specific. That is why it is the route of last resort. It is not a special case, it is just the shortest prefix there is.

For example, with routes for 10.0.0.0/8, 10.1.0.0/16, 10.1.2.0/24 and 10.1.2.128/25 all in the table, the address 10.1.2.200 is inside all four. The /25 wins.

## Task 1 - Address parsing and formatting

Implement ip_parse(), ip_format(), cidr_parse() and cidr_format().

Write the parser yourself. inet_aton() is out because it accepts 010.0.0.1 and reads it as 8.0.0.1, and sscanf("%u.%u.%u.%u") is out because it happily accepts 1.2.3.4, +1.2.3.4 and 1.2.3.4junk. A parser for a router has to reject malformed input rather than guessing at it, because a guess turns into a route to the wrong place.

Every one of these must be rejected:

    1.2.3        1.2.3.4.5    1..2.3       256.1.1.1    010.0.0.1
    1.2.3.4x     1.2.3.4/33   1.2.3.4/024  1.2.3.4/     1.2.3/24

## Task 2 - Subnet arithmetic

Implement cidr_mask(), cidr_network(), cidr_broadcast(), cidr_first_host(), cidr_last_host(), cidr_host_count(), cidr_contains(), cidr_is_network() and cidr_subnet().

Each of these is a line or two of bit operations once the mask is right, so most of the work is in the edge cases:

- cidr_mask(0) must be 0. The obvious 0xFFFFFFFF << (32 - prefix) shifts by 32 when the prefix is 0, which is undefined behavior in C. On x86 the processor only looks at the low 5 bits of the shift count, so it shifts by 0 and hands you back 0xFFFFFFFF. That bug makes the default route match only 0.0.0.0, so in practice it never matches.
- The host count of a /0 is 2^32 - 2. The answer fits in a uint32_t, but the 2^32 on the way there does not, so do that arithmetic in 64 bits.
- /31 and /32 follow the rules in the Background section, not the formula.

cidr_subnet() splits a block into n equal pieces. Equal pieces only work when n is a power of two, since splitting borrows that many bits from the host part: 4 subnets borrows 2 bits, so a /24 becomes four /26 blocks. It hands back one subnet at a time instead of filling an array, because splitting 0.0.0.0/0 into 2^31 pieces is a perfectly legal request and the array would be 16 GB.

## Task 3 - Longest prefix match

Implement route_parse(), table_create(), table_destroy(), table_count(), table_add(), table_load() and table_lookup().

A forwarding table file is one route per line: the destination, the gateway, and the outgoing device. Here is the one the grading uses. Save it as tables/example.txt in your repository.

    # destination      gateway          device
    default            132.178.227.1    eno1
    132.178.227.0/25   direct           eno1
    10.0.0.0/8         10.1.0.1         eth1
    10.1.0.0/16        direct           eth1
    10.1.2.0/24        direct           eth2
    10.1.2.128/25      10.1.2.1         eth2   # a router behind eth2
    192.168.0.0/16     10.1.0.1         eth1

The first two routes are Onyx's real routing table from A4, with its interface name eno12399np0 shortened to eno1. direct means the block is on the link itself, so the packet goes straight to the destination with no gateway.

The table has to grow as routes are added, so you will need realloc(), and table_destroy() has to give all of it back. table_add() refuses a second route to the same block, which is what ip route add does, and table_load() reports the number of the first bad line so your error message can point at it.

A linear scan of the table is fine for the lookup. Make sure it does not depend on the order the routes were added in, since a lookup that returns the first match instead of the longest one passes any test whose table happens to be sorted longest first.

### Tip

A real router holding the full Internet table, about a million prefixes, does not scan a list. It walks a binary trie, one bit of the address per level, so a lookup costs at most 32 steps no matter how big the table gets. A trie is welcome here if you want the challenge, but it is not required.

## Task 4 - The command line

Your program is built as ./build/release/myapp and must take this command line:

    Usage: myapp info <address>/<prefix>
           myapp split <network>/<prefix> <count>
           myapp lookup <table-file> <address>...
The output format is fixed because it is what is checked when your project is graded, so match it exactly. info prints what you worked out for Onyx in A4:

./build/release/myapp info 132.178.227.11/25

    address    132.178.227.11
    prefix     /25
    netmask    255.255.255.128
    network    132.178.227.0
    broadcast  132.178.227.127
    first host 132.178.227.1
    last host  132.178.227.126
    hosts      126
split prints one subnet per line, lowest first:

./build/release/myapp split 192.168.1.0/24 4

    192.168.1.0/26
    192.168.1.64/26
    192.168.1.128/26
    192.168.1.192/26
lookup loads the table and prints one line per address in the same form as ip route get: via and the gateway when there is one, then dev and the device. An address with no matching route prints unreachable.

./build/release/myapp lookup tables/example.txt 132.178.227.50 8.8.8.8 10.1.2.5 10.1.2.200

    132.178.227.50 dev eno1
    8.8.8.8 via 132.178.227.1 dev eno1
    10.1.2.5 dev eth2
    10.1.2.200 via 10.1.2.1 dev eth2

The program must:

- Exit 0 on success, 1 for a bad command line or a malformed address, 2 when the table file cannot be opened or has a bad line, and 3 when an address matches no route. If one lookup run hits more than one of these, report every address and exit with 1 if any address was malformed, otherwise 3.
- Print errors to stderr, and say what was wrong. myapp: tables/bad.txt:4: not a valid route is useful, "error" is not.
- Refuse to split a block with host bits set, or a count that is not a power of two, and exit 1, since 192.168.1.5/24 is an interface address and not a network.
- Print the usage message and exit 0 when run with no arguments at all. This is what make leak runs, so it has to be a clean, successful path.

## Task 5 - Testing and coverage

Add Unity tests for every function in src/lab.h.

    `make check`

Bit manipulation is exactly the kind of code where an off-by-one survives casual testing, so test the boundaries: /0, /31, /32, 0.0.0.0 and 255.255.255.255. Beyond those, make sure you have tests for each kind of malformed input in Task 1, a lookup where several routes match and only the longest one is right, the same table added in a different order, a table with no default route, and a table file with a bad line in the middle.

tmpfile() gives you a real FILE * to hand table_load() without leaving files lying around.

Then check your coverage:

    make clean
    make all
    make report

Fix your tests until you have 100% coverage with everything passing. As in P0, you may only exclude branches originating from system or library calls, such as a realloc() that returns NULL.

## Task 6 - Leak and crash check

Run `make leak`, then `make leak-test`.
Fix every leak and every crash. Watch the error paths in particular: a table file with a bad line on line 4 still has to free the three routes before it, and the line buffer from getline().

## Task 7 - Replace README.md

Replace README.md following this example. Your Design section should explain how you handled /0, /31 and /32, and how your table finds the longest prefix.
