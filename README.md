# BGPExtrapolator
A C++ implementation of [bgp_extrapolator](https://github.com/mike-a-p/bgp_extrapolator) along with significant changes. The Python version is not currently being developed.

## Description

This program utilizes [AS](https://en.wikipedia.org/wiki/Autonomous_system_(Internet)) [relationship](http://www.caida.org/data/as-relationships/) and [BGP](https://en.wikipedia.org/wiki/Border_Gateway_Protocol) data to approximate the BGP best path selection algorithm and simulate how route announcements propagate in the internet. The Extrapolator assumes standard [Gao-Rexford (Valley Free)](https://people.eecs.berkeley.edu/~sylvia/cs268-2019/papers/gao-rexford.pdf) routing policies. It is designed to use data retrieved and stored using the [lib_bgp_data](https://github.com/jfuruness/lib_bgp_data) Python package. 

## Requirements

* g++ supporting at least c++14
* [libpqxx](http://pqxx.org/development/libpqxx/) version 4.0
* libboost  

There is a convenience script, apt-install-deps.sh, that will install these for
you on a Debian/Ubuntu system. This program is designed and tested on a Linux 
distribution and does not officially support other environments. 

## Building 

To build from source, run `make` while in the source directory. This produces
an executable `bgp-extrapolator`. 

To run unit tests, use `make test` instead. This builds an executable that runs
tests only and does not perform a full extrapolation. 

## Usage

The Extrapolator looks for a file called "`bgp.conf`" in "`/etc/bgp/`" for
credentials to access the Postgres database. This is the same location and 
format expected by [lib_bgp_data](https://github.com/jfuruness/lib_bgp_data). 
It's expected in the following format:

```
[bgp]
#Lines starting with '[' or '#' are ignored 
username = user_name
password = password123
database = bgp
host = 127.0.0.1
port = 5432

```

The Extrapolator runs under the assumption that the database is already populated with the necessary data on AS Relationships and BGP announcements to propagate. The Extrapolator pulls this data from three input tables: customer_providers, peers, and mrt_w_roas. The easiest way to get all of that data in the databse in the correct format is to use jfuruness' [lib_bgp_data](https://github.com/jfuruness/lib_bgp_data). 

When the database is set up and the config file is in place, you can run 
```
./bgp-extrapolator
```

Optional arguments:

| Parameter | Default   | Description |
| :--- | :---: | :--- |
| -i --invert-results | true | record ASNs without route to a prefix-origin (smaller results)
| -d --store-depref | false | record announcements for depreference policy (doubles normal results)
| -s --iteration-size | 500000 | max number of announcements per iteration (higher = more memory use)
| -a --announcements-table | mrt_w_roas | name of the announcements input table
| -r --results-table | extrapolation-results | name of the normal results table (if -i 0)
| -d --depref-table | depref-results | name of the depref results table (if -d 1)
| -o --inverse-results-table | extrapolation-results | name of the inverse results table

**Parameter -i**

The normal extrapolator results contain a BGP announcement for each prefix for each AS, meaning the results contain (# of prefix) * (# of ASes). This can take significant time to write to disc as well as occupying large spaces within that disc. The inverse results store the set of ASes the DID NOT receive a particular prefix-origin, significantly reducing the size of the results stored and the time required to store them.

**Parameter -d**

In order to implement a depreference policy, to check for a second-best announcement that can be used in the case of a detected hijacking, the extrapolator can store the second-best announcement for each prefix at each AS. However, this has significant overhead in terms of the size of the results stored and the time required to store them.

**Parameter -s**

The iteration size determines the maximum number of announcements that will be queried for and pulled into system memory at one time. Lower values means less memory usage and fewer CPU cache misses.

**Parameter -a**

Allows specification of the input name of the MRT announcements table table.

**Parameter -r**

Allows specification of the output name of the normal results table.

**Parameter -d**

Allows specification of the output name of the depref results table.

**Parameter -o**

Allows specification of the output name of the inverse results table.

## Classes and Structs

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

### Extrapolator

<u>Purpose:</u> Handle the logic of the announcement propagation process. The extrapolator is intended to simulate best path selection for every AS's set of announcements based upon a seed of announcement from BGP Monitors. This is the class explicitly invoked by a main function.

<u>How:</u> The extrapolator contains an instance of [ASGraph](#ASGraph) and [SQLQuerier](#SQLQuerier). 

The extrapolation process is as follows:

1. Create a directed graph of ASes based on AS relationship data in database.
2. Collect a batch of announcements that correspond to a subset of prefixes from the MRT Announcements. The size of this subset is dependent on the desired memory load of a single process.
3. Using the [AS_PATHs](https://sites.google.com/site/amitsciscozone/home/bgp/bgp-as_path-attribute) of this batch of announcements collected by monitors, give announcements to ASes found on their respective AS_PATHs. These seeded announcements are known to have traveled there.
4. For better approximation, if (via AS_PATH) we know that an announcement was sent to a peer or provider of an AS it was likely sent to all other peers and providers of that AS. For this reason we do just that before continuing. 
5. Following ordered ranking (See [ASGraph](#ASGraph)), announcements are propagated upwards starting at the bottom (customers) and working their way towards the top (providers).
5. Again following ordered ranking, announcements are propagated out to peers.
6. Again following ordered ranking, announcements are propagated downwards from the top and working their way to the bottom.
7. Once propagation is completed for the current batch of prefixes, the results are saved to a results table in the database. 
8. Clear all announcement data from the AS Graph.
9. Repeat steps 2-8 for desired iterations or until all prefixes have been used.

### SQLQuerier

<u>Purpose:</u> The Querier struct serves as an interface between the program and the database. 

<u>How:</u> It's function calls provide SQL queries to the database and return responses. To do so it uses [pqxx](https://github.com/jtv/libpqxx), the official C++ Client API for PostgreSQL.

(See SQLQuerier.h/cpp)
