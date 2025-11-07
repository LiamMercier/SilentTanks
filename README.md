# Silent Tanks

Silent Tanks is a turn based strategy game centered around toy tanks with limited vision.

This repository contains an asynchronous multithreaded server and QML client for playing Silent Tanks.

## Table of contents

- [Installation (client)](#installation-client)
    - [Windows](#windows)
    - [Linux (Debian based)](linux-debian-based)
    - [Other users](#other-users)
- [Installation (server)](#installation-server)
    - [Windows](#windows-1)
    - [Linux (Debian based)](linux-debian-based-1)
    - [Other users](#other-users-1)
- [Compilation (Debian based)](#compilation-debian-based)
- [Compilation (Windows)](#compilation-windows)
- [Compilation (Other platforms)](#compilation-other-platforms)
- [Manual setup (Client)](#manual-setup-client)
- [Manual setup (Server)](#manual-setup-server)
- [Server hosting troubleshooting](#server-hosting-troubleshooting)
    - [Finding your private address](#finding-your-private-address)
    - [Allowing connections on the server device](#allowing-connections-on-the-server-device)
    - [How do people connect to my server?](#how-do-people-connect-to-my-server)
- [Dependencies and licenses](#dependencies-and-licenses)

## Installation (client)

### Windows

Download and run the installer from the current release. Leave the install directory as default during installation.

You should now be able to run the application from the start menu.

### Linux (Debian based)

Open a terminal in the folder containing the packages.

Install the common package.

```
sudo apt install ./silent-tanks-X.Y.Z-Linux-common.deb
```

Install the client package.

```
sudo apt install ./silent-tanks-X.Y.Z-Linux-client.deb
```

The client can now be run from the start menu assuming your desktop environment supports desktop entries. For terminal logging you can manually run the application.

```
SilentTanks-Client
```

### Other users

The client is not packaged for .rpm or other package formats, self compilation and setup will be required. Follow the compilation and manual setup steps.

## Installation (server)

### Windows

Untested.

Users should install windows subsystem for linux (WSL) and read the following section.

### Linux (Debian based)

Open a terminal in the folder containing the packages and install the common package.

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

Next, setup the postgres database by calling the database setup script.

```
sudo /usr/share/silent-tanks/setup/setup-database.sh
```

You should now be able to start the server through the terminal.

```
sudo silent-tanks-server --address <YOUR_IP>
```

The server can also be run on a different port.

```
sudo silent-tanks-server --address <YOUR_IP> --port <YOUR_PORT>
```

### Other users

The server is not packaged for .rpm or other package formats, self compilation and setup will be required. Follow the compilation and manual setup steps.

## Compilation (Debian based)

The client and server are setup to produce .deb packages. To start, ensure all project dependencies and make tools are installed.

```
sudo apt install git build-essential cmake libargon2-dev libssl-dev qt6-base-dev qt6-declarative-dev libpqxx-dev libsodium-dev libboost-dev libboost-system-dev libboost-program-options-dev
```

Download the repository and change directories to the project root.

```
git clone https://github.com/LiamMercier/SilentTanks.git
```

```
cd SilentTanks
```

Compile with the following.

```
cmake -DCMAKE_BUILD_TYPE=Release -DTARGET_PLATFORM=debian -DTARGET_APP_TYPE=gui -B builds -H. && cmake --build builds --target package
```

Optionally, compile with N threads using the following.

```
cmake -DCMAKE_BUILD_TYPE=Release -DTARGET_PLATFORM=debian -DTARGET_APP_TYPE=gui -B builds -H. && cmake --build builds -j <N> --target package
```

You should now see three debian packages in the builds directory.

Optionally if releasing to others fetch the checksum. This may not be the same as the repository release checksum because the builds are not setup to be deterministic.

```
sha256sum builds/silent-tanks-*
```

## Compilation (Windows)

This project does not compile the server for windows, though it could be possible to do so with manual configuration. To create the client, do the following.

Download [MSYS2](msys2.org). Verify the domain.

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

Copy the other required dll files and their licenses into the staging folder.

```
./packaging/windows/copy-dlls.sh builds/src/client/SilentTanks-Client.exe staging/
```

You will likely see three WARNING lines corresponding to the icu library components, since they are not included in the licenses directory. If so, copy the license from the version being packaged.

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

Optionally if releasing to others fetch the checksum. This may not be the same as the repository release checksum because the builds are not setup to be deterministic.

```
sha256sum silent-tanks-X.Y.Z-win64.exe
```

## Compilation (Other platforms)

The client and server code should work on linux based systems, but must be manually packaged or run standalone.

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
cmake -DCMAKE_BUILD_TYPE=Release -DTARGET_PLATFORM=linux -DTARGET_APP_TYPE=gui -B builds -H. && cmake --build builds
```

Now you will have an executable that you can package or otherwise use.

## Manual setup (Client)

To setup the client manually on a linux system, you need to place mapfile.txt, envs, and server-list.txt into /home/YOUR_USERNAME/.local/share/silent-tanks since this is where the client expects these files.

To setup the desktop entry and icon, assuming you are in the project root, do

```
cp packaging/linux/silent-tanks.desktop /usr/share/applications
```

```
cp qml/svgs/icons/silent-tanks.svg /usr/share/icons/hicolor/scalable/apps
```

And make it so the .desktop target SilentTanks-Client is executable.

## Manual setup (Server)

Any files that the root CMakeLists.txt and src/server/CMakeLists.txt install should be moved manually.

The .deb package mainly calls the `postinst` shell script to move other files as root, create a server user, etc. 

Either try using this shell script, or follow it and use equivalent commands if some are not supported in your distribution.

After this, you should be able to use the same setup scripts as the debian setup.

## Server hosting troubleshooting

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

Find your private address and your router's public address (possibly by logging into your router or using an external site). Enable port forwarding for your port to your private address. This server uses TCP for packets.

You may also need to alter your firewall rules (on your router, server device, or both) to allow traffic on the port.

### How do people connect to my server?

Server identities are just text that can be saved in the GUI client. These are stored in `server-list.txt` with each row representing a server.

While your server is running, you can print the identity string with `showidentity` in the command line interface.

`[Private Address]:Port:Fingerprint`

If people outside of your private network will connect to your server, you must replace the ip address with your public ip address. Private addresses are fine for LAN servers.

Server identities have two formats, with and without name metadata. Both can be pasted into the client to automatically save a server.

`[Address]:Port:Fingerprint`

`{Server Name}:[Address]:Port:Fingerprint`

If you have a domain name, you can have the user enter the domain and they will be able to connect assuming your server's certificate is signed by a source recognized by the client. Otherwise, replace the address in the identity string with your domain name.

## Dependencies and licenses

You can find the licenses for this project's dependencies in the THIRD_PARTY_LICENSES folder. Windows installs will have additional licenses from the numerous dynamic libraries that are to be brought over for the runtime. You can find a copy of every license available for these projects in the same directory after installing on windows with the understanding that the least restrictive license is being used (though all are compatible with this project's license). Most of these are from qt6 (and thus duplicated in the respective qt6 license folders).

The following table enumerates which dependencies are being targeted, with a known good version for compilation.

| Dependency | Tested version | Debian package | License |
|------------|----------------|----------------|---------|
| Boost | 1.83.0 | libboost-all-dev | Boost | Boost Software License 1.0 |
| Argon2 | 0~20190702+dfsg-4+b2 | libargon2-dev | CC0 1.0 |
| OpenSSL | 3.5.1-1 | libssl-dev | Apache 2.0 |
| libpqxx | 7.10.0-2 | libpqxx-ev | BSD-3 |
| Glaze | 6.0.2 | N/A (pulled by CMake) | MIT |
| Sodium | 1.0.18-1 | libsodium-dev | ISC license |
| Qt | 6.8.2 | qt6-base-dev <br> qt6-declarative-dev | LGPL 3.0 |
