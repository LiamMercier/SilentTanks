# Silent Tanks

TODO: about

TODO: table of contents

## Installation (client)

### Windows

TODO: fill when windows is available

### Linux (Debian based)

Open a terminal in the folder containing the packages.

Install the common package.

`sudo apt install ./silent-tanks-X.Y.Z-Linux-common.deb`

Install the client package.

`sudo apt install ./silent-tanks-X.Y.Z-Linux-client.deb`

The client can now be run with.

`SilentTanks-Client`

### Other users

The client is not packaged for .rpm or other package formats, self compilation and setup will be required. Follow the compilation and manual setup steps.

## Installation (server)

### Windows

Users should install windows subsystem for linux (WSL) and read the following section.

### Linux (Debian based)

Open a terminal in the folder containing the packages and install the common package.

`sudo apt install ./silent-tanks-X.Y.Z-Linux-common.deb`

Install the server package.

`sudo apt install ./silent-tanks-X.Y.Z-Linux-server.deb`

Ensure postgres is installed.

`sudo apt install postgresql`

Now either generate a self signed .key and .crt file with.

`sudo /usr/share/silent-tanks/setup/create-self-signed-cert.sh <YOUR_IP> <(optional) YOUR_DOMAIN>`

Or copy your existing files to /var/lib/silent-tanks/certs before continuing.

Next, setup the postgres database by calling the database setup script.

`sudo /usr/share/silent-tanks/setup/setup-database.sh`

You should now be able to start the server through the terminal.

`sudo silent-tanks-server --port <YOUR_PORT> --address <YOUR_IP>`

### Other users

The server is not packaged for .rpm or other package formats, self compilation and setup will be required. Follow the compilation and manual setup steps.

## Compilation (Debian based)

The client and server are setup to produce .deb packages. To start, ensure all project dependencies are installed.

TODO: list dependencies

Install build-essentials if necessary.

TODO: list build-essentials and other related programs needed

Download the repository and change directories to the project root.

TODO: git commands to grab repository AND test it on fresh install to ensure working

`cd SilentTanks`

Compile with the following.

`cmake -DCMAKE_BUILD_TYPE=Release -DTARGET_PLATFORM=debian -B builds -H. && cmake --build builds --target package`

Optionally, compile with N threads using the following.

`cmake -DCMAKE_BUILD_TYPE=Release -DTARGET_PLATFORM=debian -B builds -H. && cmake --build builds -j N --target package`

You should now see three debian packages in the builds directory.

## Compilation (Windows)

This project only targets windows for the client software.

TODO: windows client compilation on windows

## Compilation (Other users)

The client and server code should work on linux based systems, but must be manually packaged or run standalone.

TODO: give general advice for compilation

## Manual setup

TODO: manual setup tips for windows and linux

## Dependencies and licenses

TODO: list dependencies and licenses (mostly mit/bsd)
