# BGPExtrapolator
A C++ implementation of [bgp_extrapolator](https://github.com/mike-a-p/bgp_extrapolator) along with significant changes. The Python version is not currently being developed.

## Description

This program utilizes [AS](https://en.wikipedia.org/wiki/Autonomous_system_(Internet)) [relationship](http://www.caida.org/data/as-relationships/) and [BGP](https://en.wikipedia.org/wiki/Border_Gateway_Protocol) data to approximate what BGP data will be kept by any and all ASes in a given timeframe. It is designed to use data retrieved and stored using the [lib_bgp_data](https://github.com/jfuruness/lib_bgp_data) Python package. 

## Requirements

* g++ supporting at least c++14
* [libpqxx](http://pqxx.org/development/libpqxx/) version 4.0

## Building 

If using the source files, start by running the included Makefile to compile the program. Currently, this is done by simply running `make test` while in the source file directory after calling desired test functions in main.cpp from Tests.h/Tests.cpp .

This produces an executable `bgp-extrapolator` 

## Usage

This program is designed and tested on a Linux distribution and does not officially support other environments. 



The program does not currently support flags and other args but will in the near future.

The SQLQuerier struct currently looks for a file "`bgp.conf`" in "`/etc/bgp/`" for credentials regarding the Postgres database. This is the same location and format expected by [lib_bgp_data](https://github.com/jfuruness/lib_bgp_data). It's expected in the following format:

```
[bgp]
#Lines starting with '[' or '#' are ignored 
username = user_name
password = password123
database = bgp
host = 127.0.0.1
port = 5432

```







## Classes and Structs

### Extrapolator

<u>Purpose:</u> Handle the logic of the announcement propagation process. This is typically the class explicitly invoked by a main function.

<u>How:</u> The extrapolator contains an instance of [ASGraph](#ASGraph) and [SQLQuerier](#SQLQuerier). 

The extrapolation process is generally as follows:

1. Create graph of ASes based on AS relationship data in database.
2. Prepare a batch of announcements that correspond to a subset of prefixes. The size of this subset is dependent on the desired memory load of a single process.
3. Using the [AS_PATHs](https://sites.google.com/site/amitsciscozone/home/bgp/bgp-as_path-attribute) of announcements collected by monitors, give announcements to ASes found on their respective AS_PATHs since announcements are known to have traveled there.
4. For better approximation, if (via AS_PATH) we know that an announcement was sent to a peer or provider of an AS it was likely sent to all other peers and providers of that AS. For this reason we do just that before continuing. 
5. Following ordered ranking (See [ASGraph](#ASGraph)), announcements are propagated upwards starting at the bottom and working their way towards the top.
6. Again following ordered ranking, announcements are propagated downwards from the top and working their way to the bottom.
7. Save results has multiple options currently. Results being the announcements received and kept by every AS. One (old) option that can likely be found commented out is to save to disk in [Pandas](https://pandas.pydata.org/) readable CSV. This has since been replaced by a method that writes to a CSV in memory before copying it to a Postgres table. This is the fastest known way to make bulk inserts. 
8. Clear announcement related data from ASes.
9. Repeat steps 2-8 for desired iterations or until all prefixes have been used.

### SQLQuerier

<u>Purpose:</u> The Querier struct serves as an interface between the program and the database. 

<u>How:</u> It's function calls provide SQL queries to the database and return responses. To do so it uses [pqxx](https://github.com/jtv/libpqxx), the official C++ Client API for PostgreSQL.

(See SQLQuerier.h/cpp)

### ASGraph

<u>Purpose:</u> The ASGraph struct is responsible for creating and keeping track of all [ASes](#AS). 

<u>How:</u> First with the help of [Querier](#Querier), AS relationship data is retrieved from a local database and is used to create and modify ASes. ASes are then grouped topographically. To do so each AS is given a rank that essentially represents it's maximum distance to the "bottom" of the AS network.

The global network of ASes has a tree-like shape, where the shape is wider towards the bottom (ASes with few to no customers) and slimmer toward the top (ASes with few to no providers). With this understanding, we can define top and bottom ASes as having no providers or customers respectively.

(See ASGraph.h/cpp)

### AS

<u>Purpose:</u> The AS struct represents an autonomous system (AS). 

<u>How:</u> The AS struct keeps track of the other ASes it has relationships to as well as BGP announcements it wants to keep, discarding those it prefers less.

 (See AS.h/cpp)

### Announcement

<u>Purpose:</u> Represent a BGP announcement sent between ASes to advertise prefix-origin pairs. 

<u>How:</u> An announcement instance keeps record of the [prefix](#Prefix) it advertises, the origin AS advertising it, and data relevant to the decision making process of whether or not this announcement is preferred over another. 

### Prefix

<u>Purpose:</u> A small and fast to read representation of the prefix advertised by an announcement.

<u>How:</u> IP address and netmask regarding a prefix are stored as separate unsigned 32 bit integers.

(See Prefix.h)

