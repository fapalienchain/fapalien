Fapalien Core
==========

This is the official reference wallet for Fapalien digital currency and comprises the backbone of the Fapalien peer-to-peer network. You can [download Fapalien Core](https://www.fapalienchain.com/downloads/) or [build it yourself](#building) using the guides below.

Running
---------------------
The following are some helpful notes on how to run Fapalien on your native platform.

### Unix

Unpack the files into a directory and run:

- `bin/fapalien-qt` (GUI) or
- `bin/fapaliend` (headless)

### Windows

Unpack the files into a directory, and then run fapalien-qt.exe.

### OS X

Drag Fapalien-Qt to your applications folder, and then run Fapalien-Qt.

### Need Help?

* See the [Fapalien documentation](https://docs.fapalienchain.com)
for help and more information.
* See the [Fapalien Developer Documentation](https://fapalien-docs.github.io/) 
for technical specifications and implementation details.
* Ask for help on [Fapalien Nation Discord](http://fapalienchat.org)
* Ask for help on the [Fapalien Forum](https://fapalienchain.com/forum)

Building
---------------------
The following are developer notes on how to build Fapalien Core on your native platform. They are not complete guides, but include notes on the necessary libraries, compile flags, etc.

- [OS X Build Notes](build-osx.md)
- [Unix Build Notes](build-unix.md)
- [Windows Build Notes](build-windows.md)
- [OpenBSD Build Notes](build-openbsd.md)
- [Gitian Building Guide](gitian-building.md)

Development
---------------------
The Fapalien Core repo's [root README](/README.md) contains relevant information on the development process and automated testing.

- [Developer Notes](developer-notes.md)
- [Release Notes](release-notes.md)
- [Release Process](release-process.md)
- Source Code Documentation ***TODO***
- [Translation Process](translation_process.md)
- [Translation Strings Policy](translation_strings_policy.md)
- [Travis CI](travis-ci.md)
- [Unauthenticated REST Interface](REST-interface.md)
- [Shared Libraries](shared-libraries.md)
- [BIPS](bips.md)
- [Dnsseed Policy](dnsseed-policy.md)
- [Benchmarking](benchmarking.md)

### Resources
* Discuss on the [Fapalien Forum](https://fapalienchain.com/forum), in the Development & Technical Discussion board.
* Discuss on [Fapalien Nation Discord](http://fapalienchat.org)

### Miscellaneous
- [Assets Attribution](assets-attribution.md)
- [Files](files.md)
- [Fuzz-testing](fuzzing.md)
- [Reduce Traffic](reduce-traffic.md)
- [Tor Support](tor.md)
- [Init Scripts (systemd/upstart/openrc)](init.md)
- [ZMQ](zmq.md)

License
---------------------
Distributed under the [MIT software license](/COPYING).
This product includes software developed by the OpenSSL Project for use in the [OpenSSL Toolkit](https://www.openssl.org/). This product includes
cryptographic software written by Eric Young ([eay@cryptsoft.com](mailto:eay@cryptsoft.com)), and UPnP software written by Thomas Bernard.
