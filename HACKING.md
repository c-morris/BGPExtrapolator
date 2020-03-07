# Modifying the bgp-extrapolator

This document describes how the internals of the extrapolator work if you want
to make custom modifications to the source. 

## Classes and Structs

### Extrapolator

<u>Purpose:</u> Handle the logic of the announcement propagation process. The
extrapolator is intended to simulate propagation for every AS's set of
announcements based upon a seeded announcements from BGP Monitors. The
Extrapolator class is a singleton.

<u>How:</u> The extrapolator contains an instance of [ASGraph](#ASGraph) and [SQLQuerier](#SQLQuerier). 

The extrapolation process is as follows:

1. Create a directed graph of ASes based on AS relationship data in database.
2. Collect a batch of announcements that correspond to a subset of prefixes from the MRT Announcements. The size of this subset is dependent on the desired memory load of a single process.
3. Using the AS_PATHs of this batch of announcements collected by monitors, give announcements to ASes found on their respective AS_PATHs. These seeded announcements are known to have traveled there.
4. For better approximation, if (via AS_PATH) we know that an announcement was sent to a peer or provider of an AS it was likely sent to all other peers and providers of that AS. For this reason we do just that before continuing. 
5. Following ordered ranking (See [ASGraph](#ASGraph)), announcements are propagated upwards starting at the bottom (customers) and working their way towards the top (providers).
5. Again following ordered ranking, announcements are propagated out to peers.
6. Again following ordered ranking, announcements are propagated downwards from the top and working their way to the bottom.
7. Once propagation is completed for the current batch of prefixes, the results are saved to a results table in the database. 
8. Clear all announcement data from the AS Graph.
9. Repeat steps 2-8 for desired iterations or until all prefixes have been used.

### Prefix

<u>Purpose:</u> A compact representation of the prefix advertised by an announcement.

<u>How:</u> IPv4 address and netmask are stored as unsigned 32 bit integers.

(See Prefix.h)

### Announcement

<u>Purpose:</u> Represent a BGP announcement sent between ASes to advertise prefix-origin pairs. 

<u>How:</u> An Announcement instance keeps record of the [prefix](#Prefix) it advertises, the origin AS advertising it, and other data relevant to the best path selection process. 

(See Announcement.h)

### AS

<u>Purpose:</u> The AS struct represents an Autonomous System(AS). 

<u>How:</u> The AS struct, identified by an Autonomous System Number(ASN), keeps track of the other ASes it has relationships to as well as a set of BGP announcements it wants to keep, discarding those it prefers less. In BGP terminology, this set of preferred announcements is the Loc-RIB. 

(See AS.h/cpp)

### ASGraph

<u>Purpose:</u> The ASGraph struct is responsible for creating and keeping track of all [ASes](#AS). 

<u>How:</u> Using a [Querier](#Querier), AS relationship data is retrieved from a local database and is used to create a graph of the BGP network, with ASes as the vertices and their relationships as the edges. ASes are then grouped topographically. To do so each AS is given a rank that is its maximum distance to the "bottom" of the AS network.

The global network of ASes has a tree-like shape, where the shape is wider towards the bottom (ASes with few to no customers) and slimmer toward the top (ASes with few to no providers). With this understanding, we can define top and bottom ASes as having no providers or customers respectively.

(See ASGraph.h/cpp)

### SQLQuerier

<u>Purpose:</u> The Querier struct serves as an interface between the program and the database. 

<u>How:</u> Its function calls provide SQL queries to the database and return responses. To do so it uses [pqxx](https://github.com/jtv/libpqxx), the official C++ Client API for PostgreSQL.

(See SQLQuerier.h/cpp)
