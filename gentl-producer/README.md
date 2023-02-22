GenICam GenTL Producer
======================

The GenTL standard specifies a generic transport layer interface for
accessing cameras and other imaging devices. According to the GenICam
naming convention, a GenTL producer is a software driver that provides
access to an imaging device through the GenTL interface. A GenTL
consumer, on the other hand, is any software that uses one or more
GenTL producers through this interface. The supplied software module
represents a GenTL producer and can be used with any application
software that acts as a consumer. This allows for the the ready
integration of Nerian's devices into existing machine vision software
suites like e.g. HALCON.

Once compiled, the producer can be found in the `lib/` directory, with
the file name `nerian-gentl.cti`. In order to be found by a consumer,
this directory has to be added to the GenTL search path. The search path
is specified through the following two environment variables:

* `GENICAM_GENTL32_PATH`: Search path for 32-bit GenTL producers.
* `GENICAM_GENTL64_PATH`: Search path for 64-bit GenTL producers.

For further information on using the producer, please refer to the
appropriate sections in the user manual.

