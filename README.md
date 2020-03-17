# libems #
libems stands for event-driven master-slave. The intention of this library is
to provide a simple mechanism for master-slave applications to exchange messages.
More than that, this shall happen transparently for the application, i.e.,
it should not matter whether we use a local UNIX domain socket or a connection
via TCP.

The application can define its own messages, which have to be derived from
EMSMessage, and have to be registered via ems_message_register_type. The message
identifiers have to be provided beforehand and must be unique 31-bit integers
(messages with the high bit set are used for internal purposes).

We use the epoll interface in the socket based communicators. So this library
is Linux-only.

A small usage example is given in test.c.

There is still a lot to do. For example:
 * More error checking and better error recovery.
 * Documentation.
 * A more flexible API (e.g., change settings of connectors after creation).

This library is released under the terms of the MIT license. See LICENSE.
