## Garage alarm

This is a hobby project of garage alarm.

## Motivation

I need a simple device guarding my car in the garage. It should:
* be connected to GSM to data network and communicate over internet and over SMS (in case of alert)
* last as long as possible, because there'S not electricity there -_-
* be controllable by remote switch, as one would expect

Because I didn't found any existing device, I decided to use my expert skill in Scala and lame skills in C(++) and build my own. As OSS, of course.

## Implementation

The project has three parts:
* [Garage module](main_module.md)
* [Server module](server_module.md)
* [Remote module](remote_module.md)
