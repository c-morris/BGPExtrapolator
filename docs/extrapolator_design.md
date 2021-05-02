BGP Extrapolator Propagation Design 
====================================

::: {#objective}
Objective
---------
:::

The objectives of the extrapolator are to estimate the local routing
information base (RIB) of every AS in the internet and also to simulate
different routing policies for routing security experiments.

::: {#introduction}
Introduction
------------
:::

The BGP extrapolator propagates route announcements based on the
well-known Gao-Rexford, or valley-free routing model. This model assumes
that ASes always prefer announcements received from customers over
announcements received from peers, which are further preferred over ones
received from providers. Additionally, an AS would never announce a
route received from one provider to another provider, since it would end
up paying twice to route traffic that was not its own or its customer's.
In cases where an announcement for the same prefix is received from
multiple neighbors with the same relation, an AS prefers the
announcement with the shortest AS\_PATH.

Intuitively, this model makes sense because an AS will pay for traffic
sent to its providers, but not for traffic sent to peers, and it will
get paid for traffic sent to customers. If the only available path is
via a provider, an AS will want to announce that route to all of its
customers, however, it would never announce that route to its other
providers. Any announcement received from one of its customers is
forwarded to everyone, including other customers, peers, and providers.
This is in-line with the business incentives of an AS.

Below is a high-level overview of the algorithm.

![image](.//media/image1.png){width="6.1913in" height="3.81342in"}

::: {#steps}
Steps
-----
:::

**Building the AS Graph**

Construct an AS Graph from the CAIDA AS Relationship data. Note that the
CAIDA topology does not include any sibling relationships. In the
topology, there are several strongly connected components where, for
example, AS *x* is a customer of AS *y* and *y* is also a customer of
*x*. A strongly connected component is collapsed into a single AS, since
it behaves like single AS and all members of the component have
equivalent RIBs.

**Seeding**

For every MRT announcement, emplace that announcement into the AS Graph
at the ASes on the AS\_PATH. When an AS is prepended multiple times to
the path, the path length is incremented but the other attributes of the
announcement stay the same.

Announcements containing loops are omitted from this process.

Other deviations from normal BGP such as path poisoning, or possible
strange types of path prepending are not accounted for, so these could
result in announcements being placed where they should not be.

When using MRT traces to populate the AS graph, sometimes different MRTs
contain different announcements depending on when they were created.
Since paths in the internet are generally stable, we give preference to
the path with the oldest timestamp in cases where paths conflict. This
method will be incorrect when a path legitimately changes, however, it
filters out inconsistencies caused by events such as an MRT being
created before the RIB has stabilized after a route refresh since the
recently refreshed routes will have a newer timestamp.

**Gao Rexford Propagation and** **tiebreaking**

The Extrapolator propagates up, laterally, and then down the AS Graph
based on the following path selection criteria. The attribute at the top
of the table has the highest priority, ties are broken by comparing
attributes, starting at the top of this list. We know that paths in the
internet are generally stable, so we give older announcements higher
priority than newer ones.

1.  AS Relationship

2.  Shortest Path

3.  Oldest

4.  Random

In contrast to real BGP (steps below), we do not store information
beyond the path length, so we omit the lower three steps.

1.  AS Relationship

2.  Shortest Path

3.  Lowest Origin Type (e.g. IGP over EGP)

4.  MED and other tiebreakers

5.  Lowest BGP ID

Stub ASes, which are ASes that have no customers or peers, are omitted
from the propagation, since their routing tables will be identical to
their providers, except for the additional hop to get to that provider.
Propagation is done in batches of prefixes to save on memory.

The Extrapolator, by default, will only propagate announcements from
multi-homed ASes when none of their providers have received that
announcement. For example, a multi-homed AS which is also a collector
may report a route with itself as the origin, and in this case, it
should be propagated according to the regular propagation rules. If,
however, another collector reported a path for that same prefix and
multi-homed origin through one of its providers, then the multi-homed
origin does not send that announcement to all of its providers. This is
to acommodate for common primary-backup configurations where an AS
prefers one of its providers but maintains multiple providers for
redundancy.

The propagation logic does not treat IXPs differently than normal ASes,
even though it is known that they often do not use Gao Rexford export
policies.

::: {#constraints}
Compute Resources Constraints {#constraints}
-----------------------------
:::

Must use \< 80 GB RAM, 800 GB disk.

::: {#models}
Models
------
:::

**Priority**

The priority attribute is used internally to track the relationship and
path length of an announcement. The information is encoded into an
integer, where the first two decimal digits are used for the path length
and the third decimal is used for the relationship.

Examples:

    **Priority** **Explanation**
  -------------- ------------------------------------------------------------------------
             300 The highest priority value, a prefix originated at this AS.
             299 Announcement received from a customer (path length of one).
             298 Announcement received from a customer's customer (path length of two).
             198 Announcement received from a peer (path length of two).
              97 Announcement received from a provider (path length of three).

[ **Proposed redesign** The priority attribute is a struct with four
8-bit fields carrying information relevant to the best-path selection
process. This struct can be casted to an unsigned 32 bit integer to
efficiently compare the priority of two different announcements.
Reserved fields can be used to add other elements to the decision
process, for example, security attributes like ROA validity.
]{style="color: purple"}

``` {.C++}
struct Priority {
  // Little-endian assumed
  uint8_t reserved1;
  uint8_t path_length; // Lower precedence
  uint8_t reserved2;
  uint8_t relationship; // Highest precedence
};
```

**Results Schema**

  **Column**            **Type**
  --------------------- ----------
  ASN                   Bigint
  Prefix                CIDR
  Origin                Bigint
  Received\_from\_ASN   Bigint

**Input Schema**

  **Column**   **Type**
  ------------ ------------
  Origin       Bigint
  Prefix       CIDR
  AS\_PATH     Bigint\[\]
  Time         Bigint

**Inverse Results Schema**

  **Column**   **Type**
  ------------ ----------
  ASN          Bigint
  Prefix       CIDR
  Origin       Bigint

**Stubs Schema**

  **Column**    **Type**
  ------------- ----------
  stub\_asn     Bigint
  parent\_asn   Bigint

**\
**

**Announcement and AS struct**

+-----------------------------------------------------------------------+
| **Announcement**                                                      |
+:======================================================================+
| ``` {.C++}                                                            |
| Prefix<> prefix;            // encoded with subnet mask               |
| uint32_t origin;            // origin ASN                             |
| uint32_t priority;          // priority assigned based upon path leng |
| th and AS relation                                                    |
| uint32_t received_from_asn; // ASN that sent the ann                  |
| bool from_monitor = false;  // flag for seeded ann from MRT announcem |
| ents                                                                  |
| int64_t tstamp;             // timestamp from mrt file                |
| ```                                                                   |
+-----------------------------------------------------------------------+

+-----------------------------------------------------------------------+
| **BaseAS**                                                            |
+:======================================================================+
| ``` {.C++}                                                            |
| uint32_t asn;                                                         |
| int rank;                         // Rank in ASGraph hierarchy for pr |
| opagation                                                             |
| std::minstd_rand ran_bool;        // Random Number Generator          |
| std::vector<AnnouncementType> *incoming_announcements;                |
| std::map<Prefix<>, AnnouncementType> *all_anns;    // Maps of all ann |
| ouncements stored                                                     |
| std::map<Prefix<>, AnnouncementType> *depref_anns;                    |
|                                                                       |
| std::set<uint32_t> *providers;                                        |
| std::set<uint32_t> *peers;                                            |
| std::set<uint32_t> *customers;                                        |
|                                                                       |
| std::map<std::pair<Prefix<>, uint32_t>,std::set<uint32_t>*> *inverse_ |
| results;                                                              |
|                                                                       |
| // If this AS represents multiple ASes, its "members" are listed here |
|  (Supernodes)                                                         |
| std::vector<uint32_t> *member_ases;                                   |
| // Assigned and used in Tarjan's algorithm                            |
| int index;                                                            |
| int lowlink;                                                          |
| bool onStack;                                                         |
| bool visited;                                                         |
| ```                                                                   |
+-----------------------------------------------------------------------+
