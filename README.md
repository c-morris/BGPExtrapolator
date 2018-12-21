# BGPExtrapolator
A C++ implementation of [mike-a-p's
bgp_extrapolator](https://github.com/mike-a-p/bgp_extrapolator).

## Requirements
 * g++ supporting at least c++14
 * [libpqxx](http://pqxx.org/development/libpqxx/) version 4.0

## Building
```
make
```
will produce an executable called `bgp-extrapolator` in the project directory.

## Usage
TODO

## Running Tests
Rebuild the binary with the tests included. 
```
make clean
make test
```
Then run `./bgp-extrapolator` with no arguments to run the tests. 
