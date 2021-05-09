# BGP Extrapolator Propagation Design 

<div id="objective">

## Objective

</div>

The objectives of the extrapolator are to estimate the local routing
information base (RIB) of every AS in the internet and also to simulate
different routing policies for routing security experiments.

<div id="introduction">

## Introduction

</div>

The BGP extrapolator propagates route announcements based on the
well-known Gao-Rexford, or valley-free routing model. This model assumes
that ASes always prefer announcements received from customers over
announcements received from peers, which are further preferred over ones
received from providers. Additionally, an AS would never announce a
route received from one provider to another provider, since it would end
up paying twice to route traffic that was not its own or its customer’s.
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

![image](.//media/image1.png)

<div id="steps">

## Steps

</div>

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

**Gao Rexford Propagation and Tiebreaking**

The Extrapolator propagates up, laterally, and then down the AS Graph
based on the following path selection criteria. The attribute at the top
of the table has the highest priority, ties are broken by comparing
attributes, starting at the top of this list. Paths in the internet are
generally stable, so we give older announcements higher priority than
newer ones.

1.  AS Relationship

2.  Shortest Path

3.  Oldest

4.  Random

In contrast to real BGP shown below, we do not store information beyond
the path length, so we omit the lower three steps.

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
to accommodate for common primary-backup configurations where an AS
prefers one of its providers but maintains multiple providers for
redundancy.

The propagation logic does not treat IXPs differently than normal ASes,
even though it is known that they often do not use Gao Rexford export
policies.

<div id="constraints">

## Compute Resources Constraints

</div>

Must use \< 80 GB RAM, 800 GB disk.

<div id="models">

## Models

</div>

**Priority**

The priority attribute is used internally to track the relationship and
path length of an announcement. It is implemented as a struct with eight
8-bit fields carrying information relevant to the best-path selection
process. This struct can be casted to an unsigned 64 bit integer to
efficiently compare the priority of two different announcements.
Reserved fields can be used to add other elements to the decision
process, for example, security attributes like ROA validity.

**Results Schema**

| **Column**          | **Type** |
| :------------------ | :------- |
| ASN                 | Bigint   |
| Prefix              | CIDR     |
| Origin              | Bigint   |
| Received\_from\_ASN | Bigint   |

**Input Schema**

| **Column** | **Type**                             |
| :--------- | :----------------------------------- |
| Origin     | Bigint                               |
| Prefix     | CIDR                                 |
| AS\_PATH   | Bigint<span>\[</span><span>\]</span> |
| Time       | Bigint                               |

**Inverse Results Schema**

| **Column** | **Type** |
| :--------- | :------- |
| ASN        | Bigint   |
| Prefix     | CIDR     |
| Origin     | Bigint   |

**Stubs Schema**

| **Column**  | **Type** |
| :---------- | :------- |
| stub\_asn   | Bigint   |
| parent\_asn | Bigint   |

**  
**

**Announcement and AS struct**

<table>
<thead>
<tr class="header">
<th style="text-align: left;"><strong>Announcement</strong></th>
</tr>
</thead>
<tbody>
<tr class="odd">
<td style="text-align: left;"><div class="sourceCode" id="cb1"><pre class="sourceCode C++"><code class="sourceCode cpp"><a class="sourceLine" id="cb1-1" title="1">Prefix&lt;&gt; prefix;            <span class="co">// encoded with subnet mask</span></a>
<a class="sourceLine" id="cb1-2" title="2"><span class="dt">uint32_t</span> origin;            <span class="co">// origin ASN</span></a>
<a class="sourceLine" id="cb1-3" title="3"><span class="dt">uint32_t</span> priority;          <span class="co">// priority assigned based upon path length and AS relation</span></a>
<a class="sourceLine" id="cb1-4" title="4"><span class="dt">uint32_t</span> received_from_asn; <span class="co">// ASN that sent the ann</span></a>
<a class="sourceLine" id="cb1-5" title="5"><span class="dt">bool</span> from_monitor = <span class="kw">false</span>;  <span class="co">// flag for seeded ann from MRT announcements</span></a>
<a class="sourceLine" id="cb1-6" title="6"><span class="dt">int64_t</span> tstamp;             <span class="co">// timestamp from mrt file</span></a></code></pre></div></td>
</tr>
</tbody>
</table>

<table>
<thead>
<tr class="header">
<th style="text-align: left;"><strong>BaseAS</strong></th>
</tr>
</thead>
<tbody>
<tr class="odd">
<td style="text-align: left;"><div class="sourceCode" id="cb1"><pre class="sourceCode C++"><code class="sourceCode cpp"><a class="sourceLine" id="cb1-1" title="1"><span class="dt">uint32_t</span> asn;     </a>
<a class="sourceLine" id="cb1-2" title="2"><span class="dt">int</span> rank;                         <span class="co">// Rank in ASGraph hierarchy for propagation </span></a>
<a class="sourceLine" id="cb1-3" title="3"><span class="bu">std::</span>minstd_rand ran_bool;        <span class="co">// Random Number Generator</span></a>
<a class="sourceLine" id="cb1-4" title="4"><span class="bu">std::</span>vector&lt;AnnouncementType&gt; *incoming_announcements;</a>
<a class="sourceLine" id="cb1-5" title="5"><span class="bu">std::</span>map&lt;Prefix&lt;&gt;, AnnouncementType&gt; *all_anns;    <span class="co">// Maps of all announcements stored</span></a>
<a class="sourceLine" id="cb1-6" title="6"><span class="bu">std::</span>map&lt;Prefix&lt;&gt;, AnnouncementType&gt; *depref_anns;</a>
<a class="sourceLine" id="cb1-7" title="7"></a>
<a class="sourceLine" id="cb1-8" title="8"><span class="bu">std::</span>set&lt;<span class="dt">uint32_t</span>&gt; *providers; </a>
<a class="sourceLine" id="cb1-9" title="9"><span class="bu">std::</span>set&lt;<span class="dt">uint32_t</span>&gt; *peers; </a>
<a class="sourceLine" id="cb1-10" title="10"><span class="bu">std::</span>set&lt;<span class="dt">uint32_t</span>&gt; *customers; </a>
<a class="sourceLine" id="cb1-11" title="11"></a>
<a class="sourceLine" id="cb1-12" title="12"><span class="bu">std::</span>map&lt;<span class="bu">std::</span>pair&lt;Prefix&lt;&gt;, <span class="dt">uint32_t</span>&gt;,<span class="bu">std::</span>set&lt;<span class="dt">uint32_t</span>&gt;*&gt; *inverse_results; </a>
<a class="sourceLine" id="cb1-13" title="13"></a>
<a class="sourceLine" id="cb1-14" title="14"><span class="co">// If this AS represents multiple ASes, its &quot;members&quot; are listed here (Supernodes)  </span></a>
<a class="sourceLine" id="cb1-15" title="15"><span class="bu">std::</span>vector&lt;<span class="dt">uint32_t</span>&gt; *member_ases;</a>
<a class="sourceLine" id="cb1-16" title="16"><span class="co">// Assigned and used in Tarjan&#39;s algorithm</span></a>
<a class="sourceLine" id="cb1-17" title="17"><span class="dt">int</span> index;</a>
<a class="sourceLine" id="cb1-18" title="18"><span class="dt">int</span> lowlink;</a>
<a class="sourceLine" id="cb1-19" title="19"><span class="dt">bool</span> onStack;</a>
<a class="sourceLine" id="cb1-20" title="20"><span class="dt">bool</span> visited;      </a></code></pre></div></td>
</tr>
</tbody>
</table>
