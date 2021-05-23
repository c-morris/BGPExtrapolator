# BGPExtrapolator
Propagate BGP Announcements through an AS Graph for RIB Estimation. 

## Description

This program utilizes
[AS](https://en.wikipedia.org/wiki/Autonomous_system_(Internet))
[relationship](http://www.caida.org/data/as-relationships/) and
[BGP](https://en.wikipedia.org/wiki/Border_Gateway_Protocol) data to
approximate the BGP best path selection algorithm and simulate how route
announcements propagate in the internet. The Extrapolator assumes standard
[Gao-Rexford (Valley
Free)](https://people.eecs.berkeley.edu/~sylvia/cs268-2019/papers/gao-rexford.pdf)
routing policies. It is designed to use data retrieved and stored using the
[lib_bgp_data](https://github.com/jfuruness/lib_bgp_data) Python package. 

## Requirements

* g++ supporting at least c++14
* [libpqxx](http://pqxx.org/development/libpqxx/) version 4.0
* libboost  

To install dependencies on Ubuntu/Debian:
```
sudo apt install build-essential make libboost-dev libboost-test-dev libboost-program-options-dev libpqxx-dev libboost-filesystem-dev libboost-log-dev libboost-thread-dev libpq-dev
```

The extrapolator is designed and tested on an Ubuntu Linux distribution and
does not officially support other environments. 

## Building 

To build from source and install, run 
```
make
sudo make install
```

This produces an executable `bgp-extrapolator`. 

To build for running unit tests, run `make test` in the source diretcory and
then execute the resulting bgp-extrapolator executable.

## Usage

The Extrapolator looks for an ini file "`/etc/bgp/bgp.conf`" for credentials to
access the Postgres database. This is the same location and format expected by
[lib_bgp_data](https://github.com/jfuruness/lib_bgp_data).  The following is an
example bgp.conf:

```
[bgp]
#Lines starting with '#' are ignored 
username = user_name
password = password123
database = bgp
host = 127.0.0.1
port = 5432

```

The Extrapolator runs under the assumption that the database is already
populated with the necessary data on AS Relationships to create the AS topology
and BGP MRT announcements to propagate. The Extrapolator pulls this data from
three input tables: customer_providers, peers, and mrt_w_roas. The easiest way
to get all of this data into the databse with the correct schema is to use
jfuruness' [lib_bgp_data](https://github.com/jfuruness/lib_bgp_data). 

The table schema are laid out below:

TODO

When the database is set up and the config file is in place, you can run 
```
bgp-extrapolator
```

Optional arguments:

| Parameter | Default   | Description |
| :--- | :---: | :--- |
| -v --rovpp-run | false | flag for rovpp run
| -f --rovpp-simulation-table | simulation_announcements | name of simulation announcements table
| -u --rovpp-tracked-ases-table | tracked_ases | name of tracked ases table for attackers and victims
| -t --rovpp-policy-tables | vector<string>>() | space-separated names of ROVpp policy tables
| -k --rovpp-prop-twice | true | flag whether or not to propagate twice
| -z --ezBGPsec-run-rounds | 0 | a dual purpose integer that tells how many ezBGPsec rounds to simulate (0 means it will not run ezBGPsec at all)
| -n --ezBGPsec-intermediate | 0 | the constant for the number of "in-between" ASes an attacker will fabricate between it and the origin
| -b --random-tiebraking | true | a flag for random tiebraking for choosing announcements. True is a closer approximation, false is for testing
| -i --invert-results | true | record ASNs without route to a prefix-origin (smaller results)
| -d --store-depref | false | record announcements for depreference policy (doubles normal results)
| -s --iteration-size | 50000 | max number of announcements per iteration (higher = more memory use)
| -a --announcements-table | mrt_w_roas | name of the announcements input table
| -r --results-table | extrapolation-results | name of the normal results table (if -i 0)
| -d --depref-table | depref-results | name of the depref results table (if -d 1)
| -o --inverse-results-table | extrapolation-inverse-results | name of the inverse results table
| -l --log-folder | disabled | enables the logger and specifies a folder to save log files

**-v**
This tells whether or not to run the ROV version of the extrapolator. There can only be one of these flags out of the different versions of the extrapolator built in this framework,

**-f**
Allows specification of the name of the simulation announcements table

**-u**

Allows specification of the name of the tracked ases table for attackers and victims

**-t**

A list of space-separated names of ROVpp policy tables.

**-z**

This constant has two purposes, specify the number of ezBGPsec rounds and enable the EZBGPsec portion of the project. If the round number is equal to 0, then EZBGPsec will not run.

**-n**

This is for intermediate attacks in EZBGPsec. This specifies how many ASes an attacker will make up between it and the origin (the intent is to shift the blame and avoid Community Detection). This only modifies the priority of the announcement.

**-b**

Sometimes an AS has a tie between two different announcements for the same prefix. This field decides whether the tiebrake is random or not. The random tiebrake is probably closer to reality, but unhelpful during testing when consistent results are desired.

**-i**

The extrapolator results contain a BGP announcement for each prefix for each AS, meaning the results contain (# of prefixes) * (# of ASes). This can consume a lot of disk space. The inverse results take advantage of the fact that most ASes have routes to most other ASes and store the set of ASes the _did not_ receive a particular prefix-origin, significantly reducing the size of the results.

**-d**

In order to implement a depreference policy, to check for a second-best announcement that can be used in the case of a detected hijacking, the extrapolator can store the second-best announcement for each prefix at each AS. This has significant overhead in terms of the size of the results stored and the time required to store them.

**-s**

The iteration size determines the maximum number of announcements that will be queried for and pulled into system memory at one time. Lower values means less memory usage and better CPU cache performance.

**-a**

Allows specification of the input name of the MRT announcements table table.

**-r**

Allows specification of the output name of the normal results table.

**-d**

Allows specification of the output name of the depref results table.

**-o**

Allows specification of the output name of the inverse results table.

**-l**

With this flag and a specified directory, the logger will be enabled and generate files in the specified directory. A directory MUST be specified. Everytime the logger runs, it will remove all .log files in the directory.


