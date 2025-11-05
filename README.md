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

This project does not compile the server for windows, though it could be possible to do so with manual configuration. To create the client, do the following.

Download [MSYS2](msys2.org). Verify the domain.

Open the UCRT64 environment and update.

`pacman -Syu`

`pacman -Su`

Install the toolchain and dependencies.

`pacman -S git mingw-w64-ucrt-x86_64-toolchain mingw-w64-ucrt-x86_64-qt6-base mingw-w64-ucrt-x86_64-qt6-declarative mingw-w64-ucrt-x86_64-qt6-svg mingw-w64-ucrt-x86_64-qt6-multimedia mingw-w64-ucrt-x86_64-boost mingw-w64-ucrt-x86_64-argon2 mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-nsis`

Grab the repository.

`git clone https://github.com/LiamMercier/SilentTanks.git`

Compile.

`cd SilentTanks`

`cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DTARGET_PLATFORM=windows -S . -B builds && cmake --build builds`

Add qmlimportscanner.exe to the path.

`export PATH="/ucrt64/share/qt6/bin:$PATH"`

Call windeployqt6.

`windeployqt6 --release --qmldir "${PWD}/qml" --dir staging "${PWD}/builds/src/client/SilentTanks-Client.exe"`

Copy the other required dll files into the staging folder.

`./packaging/windows/copy-dlls.sh builds/src/client/SilentTanks-Client.exe staging/`

Package into an executable.

`cd builds`

`cpack -G NSIS`

Optionally if releasing to others fetch the checksum. This may not be the same as the repository release checksum because the builds are not setup to be deterministic.

`sha256sum silent-tanks-X.Y.Z-win64.exe`

## Compilation (Other platforms)

The client and server code should work on linux based systems, but must be manually packaged or run standalone.

TODO: give general advice for compilation

## Manual setup

TODO: manual setup tips for windows and linux

## Server hosting troubleshooting

### Finding your address

In a terminal.

`ip addr show`

TODO help less technical users

### Allowing connections on the server device

## Dependencies and licenses

TODO: list dependencies and licenses (mostly mit/bsd)
