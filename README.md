# Silent Tanks

This repository contains an asynchronous multithreaded server and QML client for playing Silent Tanks, a turn based strategy game centered around toy tanks with limited vision.

## Table of contents

- [Installation (Client)](#installation-client)
    - [Windows](#windows)
    - [Linux (Debian based)](#linux-debian-based)
    - [Other Users](#other-users)
- [Installation (Server)](#installation-server)
    - [Windows](#windows-1)
    - [Linux (Debian based)](#linux-debian-based-1)
    - [Other Users](#other-users-1)
- [Compilation (Debian based)](#compilation-debian-based)
- [Compilation (Windows)](#compilation-windows)
- [Compilation (Other Platforms)](#compilation-other-platforms)
- [Manual Setup (Client)](#manual-setup-client)
- [Manual Setup (Server)](#manual-setup-server)
- [Server Hosting Troubleshooting](#server-hosting-troubleshooting)
    - [Finding your private address](#finding-your-private-address)
    - [Allowing connections on the server device](#allowing-connections-on-the-server-device)
    - [How do people connect to my server?](#how-do-people-connect-to-my-server)
- [Dependencies and Licenses](#dependencies-and-licenses)
- [Design and Features](#design-and-features)
    - [Server Design](#server-design)
    - [Possible Server Improvements](#possible-server-improvements)
    - [Client Design](#client-design)
    - [Possible Client Improvements](#possible-client-improvements)

## Installation (Client)

### Windows

Download and run the installer from the current release. Leave the install directory as default during installation.

You should now be able to run the application from the start menu.

### Linux (Debian based)

Open a terminal in the directory containing the packages.

Install the common package.

```
sudo apt install ./silent-tanks-X.Y.Z-Linux-common.deb
```

Install the client package.

```
sudo apt install ./silent-tanks-X.Y.Z-Linux-client.deb
```

The client can now be run from the start menu assuming your desktop environment supports desktop entries.

For terminal logging you can manually run the application.

```
SilentTanks-Client
```

### Other Users

The client is not packaged for .rpm or other package formats, self compilation and setup will be required. Follow the compilation and manual setup steps.

## Installation (Server)

### Windows

Untested.

Users could try to use Windows Subsystem for Linux (WSL) or a debian based virtual machine and read the following section.

### Linux (Debian based)

Open a terminal in the directory containing the packages and install the common package.

```
sudo apt install ./silent-tanks-X.Y.Z-Linux-common.deb
```

Install the server package.

```
sudo apt install ./silent-tanks-X.Y.Z-Linux-server.deb
```

Ensure postgres is installed.

```
sudo apt install postgresql
```

Now either generate a self signed .key and .crt file with.

```
sudo /usr/share/silent-tanks/setup/create-self-signed-cert.sh <YOUR_IP> <(optional) YOUR_DOMAIN>
```

Or copy your existing files to /var/lib/silent-tanks/certs before continuing.

Next, set up the postgres database by calling the database set up script.

```
sudo /usr/share/silent-tanks/setup/setup-database.sh
```

Optionally you can change the password to the database and modify the `.pgpass` file that was left in your home directory. This is also where you can find the randomly generated password for the database if manual actions are necessary.

You should now be able to start the server through the terminal.

```
sudo silent-tanks-server --address <YOUR_IP>
```

The server can also be run on a different port.

```
sudo silent-tanks-server --address <YOUR_IP> --port <YOUR_PORT>
```

### Other Users

The server is not packaged for .rpm or other package formats, self compilation and set up will be required. Follow the compilation and manual set up steps.

## Compilation (Debian based)

The CMake script is set up to produce .deb packages for the client and server. To start, ensure all project dependencies and make tools are installed.

```
sudo apt install git build-essential cmake libargon2-dev libssl-dev qt6-base-dev qt6-declarative-dev libpqxx-dev libsodium-dev libboost-dev libboost-system-dev libboost-program-options-dev
```

Download the repository and change to the project root.

```
git clone https://github.com/LiamMercier/SilentTanks.git
```

```
cd SilentTanks
```

Compile with the following.

```
cmake -DCMAKE_BUILD_TYPE=Release -DTARGET_PLATFORM=debian -DTARGET_APP_TYPE=gui -B builds -S . && cmake --build builds --target package
```

Optionally, you can instead compile with N threads using the following.

```
cmake -DCMAKE_BUILD_TYPE=Release -DTARGET_PLATFORM=debian -DTARGET_APP_TYPE=gui -B builds -S . && cmake --build builds -j <N> --target package
```

You should now see three debian packages in the builds directory.

If releasing to others you can fetch the checksum. This may not be the same as the repository release checksum because the builds are not setup to be deterministic.

```
sha256sum builds/silent-tanks-*
```

## Compilation (Windows)

This project does not compile the server for Windows, though it could be possible to do so with manual configuration. To create the client, do the following.

Download [MSYS2](https://msys2.org). Verify the domain.

Open the UCRT64 environment and update.

```
pacman -Syu
```

```
pacman -Su
```

Install the toolchain and dependencies.

```
pacman -S git mingw-w64-ucrt-x86_64-toolchain mingw-w64-ucrt-x86_64-qt6-base mingw-w64-ucrt-x86_64-qt6-declarative mingw-w64-ucrt-x86_64-qt6-svg mingw-w64-ucrt-x86_64-qt6-multimedia mingw-w64-ucrt-x86_64-boost mingw-w64-ucrt-x86_64-argon2 mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-nsis
```

Grab the repository.

```
git clone https://github.com/LiamMercier/SilentTanks.git
```

Compile.

```
cd SilentTanks
```

```
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DTARGET_PLATFORM=windows -DTARGET_APP_TYPE=gui -S . -B builds && cmake --build builds
```

Optionally, compile with N threads using the following.

```
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DTARGET_PLATFORM=windows -DTARGET_APP_TYPE=gui -S . -B builds && cmake --build builds -j <N>
```

Add qmlimportscanner.exe to the path.

```
export PATH="/ucrt64/share/qt6/bin:$PATH"
```

Call windeployqt6.

```
windeployqt6 --release --qmldir "${PWD}/qml" --dir staging "${PWD}/builds/src/client/SilentTanks-Client.exe"
```

Copy the other required dll files and their licenses into the staging directory.

```
./packaging/windows/copy-dlls.sh builds/src/client/SilentTanks-Client.exe staging/
```

You will likely see three WARNING lines corresponding to the icu library components, since they are not included in the licenses directory. If so, copy the license from the version being packaged, though it is likely the license directory for the Qt6 version you are using already had these.

```
mkdir -p staging/licenses/icu && cp /ucrt64/share/icu/77.1/LICENSE staging/licenses/icu
```

Package into an executable.

```
cd builds
```

```
cpack -G NSIS
```

If releasing to others you can fetch the checksum. This may not be the same as the repository release checksum because the builds are not setup to be deterministic.

```
sha256sum silent-tanks-X.Y.Z-win64.exe
```

## Compilation (other platforms)

The client and server code should work on Linux based systems, but must be manually packaged or run standalone.

To compile, you need to install cmake, a c++ compiler and the dependencies in your package manager. These are listed in [Dependencies and licenses](#dependencies-and-licenses).

Grab the repository.

```
git clone https://github.com/LiamMercier/SilentTanks.git
```

Compile.

```
cd SilentTanks
```

```
cmake -DCMAKE_BUILD_TYPE=Release -DTARGET_PLATFORM=linux -DTARGET_APP_TYPE=gui -B builds -S . && cmake --build builds
```

Now you will have an executable that you can package or otherwise use.

## Manual Setup (Client)

To set up the client manually on a Linux system, you need to place `mapfile.txt`, `envs`, and `server-list.txt` into `/home/YOUR_USERNAME/.local/share/silent-tanks` since this is where the client expects these files.

To set up the desktop entry and icon, assuming you are in the project root, do

```
cp packaging/linux/silent-tanks.desktop /usr/share/applications
```

```
cp qml/svgs/icons/silent-tanks.svg /usr/share/icons/hicolor/scalable/apps
```

And make it so the .desktop target SilentTanks-Client is executable.

## Manual Setup (Server)

Any files that the root `CMakeLists.txt` and `src/server/CMakeLists.txt` install should be moved manually.

The .deb package mainly calls the `postinst` shell script to move other files as root, create a server user, etc. 

Either try using this shell script, or follow it and use equivalent commands if some are not supported in your distribution.

After this, you should be able to use the same setup scripts as the debian setup.

## Server Hosting Troubleshooting

### Finding your private address

In a terminal.

```
ip addr show
```

You do not want the loopback address, which goes by the interface name `lo`. The loopback address is only for communication between processes on your device. You will want to look for an interface advertising itself as BROADCAST or similar, with inet 192.168.X.Y or 10.X.Y.Z as the address.

Alternatively, find which interface would reach the internet.

```
ip route get 1.1.1.1
```

### Allowing connections on the server device

You will likely need to enable port forwarding on the router to forward the traffic from your public address to your server device's private address. By default, the server uses `49656` for the port, but you can change this when you launch the server.

Find your private address and your router's public address (possibly by logging into your router or using an external site). Enable port forwarding on your desired port to the server device's private address; this software uses TCP for packets.

You may also need to alter your firewall rules (on your router, server device, or both) to allow traffic on the port.

### How do people connect to my server?

Server identities are just text that can be saved in the GUI client. These are stored in the client's `server-list.txt` with each row representing a server.

While your server is running, you can print the identity string with `showidentity` in the command line interface.

`[Private Address]:Port:Fingerprint`

If people outside of your private network will connect to your server, you must replace the private address with your public ip address. Private addresses are fine for LAN servers.

Server identities have two formats, with and without name metadata. Both can be pasted into the client's connect menu to automatically save a server.

`[Address]:Port:Fingerprint`

`{Server Name}:[Address]:Port:Fingerprint`

If you have a domain name, you can have the user enter the domain and they will be able to connect assuming your server's certificate is signed by a source recognized by the client. Otherwise, replace the address in the identity string with your domain name.

## Dependencies and Licenses

You can find the licenses for this project's dependencies in the THIRD_PARTY_LICENSES directory. Windows installs will have additional licenses from the numerous dynamic libraries that are brought over for the runtime. You can find a copy of every license available for these projects in the same directory after installing on Windows with the understanding that the least restrictive license is being used (though all are compatible with this project's license). Most of these are from qt6 (and thus duplicated in the respective qt6 license directory).

The following table enumerates which dependencies are being targeted, with a known good version for compilation.

| Dependency | Tested version | Debian package | License |
|------------|----------------|----------------|---------|
| Boost | 1.83.0 | libboost-all-dev | Boost Software License 1.0 |
| Argon2 | 0~20190702+dfsg-4+b2 | libargon2-dev | CC0 1.0 |
| OpenSSL | 3.5.1-1 | libssl-dev | Apache 2.0 |
| libpqxx | 7.10.0-2 | libpqxx-dev | BSD-3 |
| Glaze | 6.0.2 | N/A (pulled by CMake) | MIT |
| Sodium | 1.0.18-1 | libsodium-dev | ISC license |
| Qt | 6.8.2 | qt6-base-dev <br> qt6-declarative-dev | LGPL 3.0 |

## Design and Features

### Server Design

The server software is designed to be multithreaded and asynchronous, using Boost Asio to prevent blocking operations and handle workers. To promote concurrency, the server is split into individual components which are focused on different tasks, all owned by the `Server` class.

`Server` holds all client sessions (which are represented by the `Session` class and are identified by number). New client connections are asynchronously accepted, checked for IP bans, and then turned into a `Session` instance which holds the SSL socket and manages `Message` handling until the peer disconnects or becomes inactive.

Rate limiting is done using a time based token system, updated each time a client makes a request. Rate limiting is treated as a nondestructive event, the request is dropped and a short message is given to the client to tell them they must wait.

Clients must adhere to the protocol deemed acceptable by the server. If a client sends a request that is unrecognized or of malformed size, the session is immediately terminated with the assumption that the client must be faulty. Valid messages are handled by the server's `on_message` function, which is passed to each session as a callback. The `on_message` function assumes it is running in a `Session` strand and thus does not access server members without a concurrency primitive.

To mitigate slowloris style denial of service, the server sets a timer on reading message bodies and closes the connection if the client is unreasonably slow. Likewise for slow read attacks, the server sets a timer on how long a client can take to read a message being sent and enforces a maximum message backlog. To ensure sessions are still active the server periodically sends a heartbeat ping which must be responded to.

Database calls are made through the server's `Database` instance, which is the sole method of talking with the underlying PostgreSQL database. Since database operations may block, this is done using a separate thread pool to keep other server threads available. Database operations are serialized using a `KeyedExecutor` which assigns a strand to each unique user ID (UUID), preventing user data races during concurrent database calls.

User passwords stored as an argon2id hash with a per user salt. Valid login requests must contain exactly `32` bytes of data (see [cryptography-constants.h](src/protocol.cryptography-constants.h)) with the expectation that clients will also hash their password before sending it to the server to prevent a malicious (or compromised) server sniffing plaintext passwords. Usernames characters can be alphanumeric, dashes, or underscores.

On authentication the database records the current login time and IP, grabs the user's friend and block lists, fetches the elo for each game mode, then calls back to the `Server` instance. If the database call failed, the server's `on_auth` will tell the client they are not authenticated, otherwise the data gets passed to the server's `UserManager` instance and the user is sent a `GoodAuth` message.

The `UserManager` class is responsible for keeping track of which session belongs to which user ID, it stores this information in a map of UUIDs to `User` data. When a user is authenticated the `UserManager` will see if there is a match in progress to reconnect the user. If a session belonging to a user is closed and there is no match in progress, the entry is removed until the next login. 

The `UserManager` is expected to notify the user's session about elo changes and handle text messages between players. Users who are blocked by the receiver have their messages filtered during a match, and direct messages are only permitted between friends.

Matchmaking requests are handled by the server's `MatchMaker` instance. The `MatchMaker` class acts as an abstraction for each game mode's match making strategy, mapping UUIDs to any active `MatchInstance` or queued game mode to prevent the user from playing in more than one match. One `IMatchStrategy` compatible instance is held in the `MatchMaker` for each game mode, all of which are given a callback to hand the `MatchMaker` a set of players ready to be put into a match and a random map to use. Periodically a timer will go off, call `tick()` on each match strategy, then re-arm itself.

Matching for casual game modes is simple, each casual match strategy uses a queue which matches the first N players together into a game. Ranked game modes match players by elo, which is done by bucketing. Buckets are created across `ELO_FLOOR` to `MAX_ELO_BUCKET` with one overflow bucket for any users with extremely high elo values. Players are matched within their bucket first, with the search expanding up and down by one bucket for every `BUCKET_INCREMENT_TIME` that has past, up to a maximum elo difference of `MAX_BUCKETS_DIFF`.

Each `MatchInstance` acts like a state machine, holding a `GameInstance` and other information for keeping track of player turns and timers. Players take turns making moves until the game is concluded, at which point the database is given a `MatchResult` instance to record.

If a match was rated, the elo change will be calculated after recording the match results. Silent Tanks updates user elos using an N-way FFA variant of the standard elo system. Each user has a placement in the match which is used to scale how well they did individually against each opponent, which is averaged and then updated using the standard elo update. When computing the elo update for lobbies larger than 2 players, we scale the base K factor by log<sub>2</sub>(N).

User elos have no upper bound, but do have a lower bound of `500` points to prevent some users from being unable to find a match. Each elo update is bounded to prevent extreme updates, but under normal matchmaking these bounds should never be reached. Every elo update is zero sum unless the operation would set a user below the minimum elo, over time this can cause elo inflation which could be reduced manually by the operator by normalizing the elo tables (some games call this a soft reset). See [elo-updates.cpp](src/server/elo-updates.cpp) for implementation details.

Server operators can interact with the server process using the command line interface, a global `Console` instance handles logging important information to the operator and reading user commands. Available commands can be displayed by typing `Help` into the interface.

### Possible Server Improvements

There are software improvements that can be made to the server for future releases, mostly to prevent bad actors if the host does not have mitigations. 

Currently there are no ip based session limits, a client can request many sessions from the same address without the server refusing the connection unless the operator manually bans the ip address. If a malicious actor opens many sessions, this could cause the server to run out of file descriptors for sockets or other resources. Clients are also rate limited with respect to requests, but there is nothing stopping a malicious actor from quickly opening and closing connections to try and exhaust the server's acceptor or other resources.

Both of these problems could be alleviated by setting up some sort of IP based book keeping for how sessions are behaving, using heuristics to throttle or temporarily ban misbehaving clients. Abstractly, a new server component could be created to track how many sessions are being opened and closed by IP addresses, which could be informed by the server's accept loop and disconnect callbacks.

Some malicious actors may have access to multiple IP addresses, each address can be used to connect the maximum allowable number of sessions which then sit idle or infrequently send commands to force CPU usage. This would reduce the effectiveness of IP related mitigations since the actor is not tied down to one source.

Further, while there are mitigations in place to prevent clients from intentionally sending or receiving their messages slowly, we cannot add a timer to the `Session` header read calls (which reads asynchronously until we see a valid `Header`). There is no difference from the server's point of view between an inactive user and an actor intentionally not sending actions, so we cannot simply remove sessions which are not actively sending requests.

Currently sessions that are inactive are forced to respond to a heartbeat ping, but this puts little burden on a malicious actor, they can simply send a few bytes back. A stronger mitigation to preventing spam or mass inactive sessions could be to force clients to solve a CPU bound challenge as part of the heartbeat, or when the client has spent the allotted amount of request tokens. Any challenge implemented would need to be fast to verify on the server but relatively slow for clients to compute, preferably in a way that is hard to parallelize and adjustable to the current server load.

The same solution could be applied to account registration. Currently a malicious actor can connect to the server, register an account, disconnect, and then repeat. Requiring clients to solve a harder challenge each time they register would reduce database load and account creation spam.

Server logs could also be saved to a file by the `Console` class. Currently, logging and command line input is not saved to disk, which could make audits difficult for server operators.

### Client Design

The client software uses Boost Asio for networking and other aspects of client-server communication; the primary architecture is a model-view-controller. The user interacts with a QML front-end, while Boost Asio is used for networking and other aspects of client-server communication within the `Client` class. These two aspects talk to one another through the `GUIClient` class, which in theory could be replaced to make a headless client if desired.

When the `Client` instance is given data which needs to be displayed to the user (such as a `PlayerView` or text message), this is given to the `GUIClient` which hands the data to the respective QObject for that data type. Requests by the user are likewise forwarded through to the underlying `Client` instance.

Passwords are hashed by the client automatically before sending to reduce the threat of a compromised or otherwise malicious server. There is no per account salt stored in the client, because a user may remove the software package and be locked out of their account. Instead, there is a randomly generated `GLOBAL_CLIENT_SALT` (see [cryptography-constants.h](src/protocol.cryptography-constants.h)) which is used by all clients. 

Using a global salt is not as good as a per account salt, but still requires a malicious server collecting passwords to compute a lookup table specific to Silent Tanks clients instead of a precomputed table. Honest servers on the other hand treat this hash as a password and do standard hashing with a salt during account creation.

### Possible Client Improvements

The client could use a way to save match replays to disk, right now they are only stored in memory and cannot be shared with others or saved for later. It would be useful to add a button to request a specific match given a match ID, then users could see other user's matches without being in their game.

While effort was put in to make a dark themed GUI, it could be better polished. There has also been minimal testing for text contrast, and a light themed GUI is not available. Decorations and better styling could be applied to various places in the GUI.
