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
#Lines starting with '[' or '#' are ignored 
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
| -i --invert-results | true | record ASNs without route to a prefix-origin (smaller results)
| -d --store-depref | false | record announcements for depreference policy (doubles normal results)
| -s --iteration-size | 50000 | max number of announcements per iteration (higher = more memory use)
| -a --announcements-table | mrt_w_roas | name of the announcements input table
| -r --results-table | extrapolation-results | name of the normal results table (if -i 0)
| -d --depref-table | depref-results | name of the depref results table (if -d 1)
| -o --inverse-results-table | extrapolation-inverse-results | name of the inverse results table
| -l --log-folder | disabled | enables the logger and specifies a folder to save log files

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

**Parameter -l**

With this flag and a specified directory, the logger will be enabled and generate files in the specified directory. A directory MUST be specified. Everytime the logger runs, it will remove all .log files in the directory.


