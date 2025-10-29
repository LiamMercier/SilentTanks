# Silent Tanks

TODO: about

TODO: table of contents

## Installation (client)

### Windows

TODO: fill when windows is available

### Linux (debian based)

Open a terminal in the folder containing the packages.

Install the common package

`sudo apt install ./silent-tanks-X.Y.Z-Linux-common.deb`

Install the client package

`sudo apt install ./silent-tanks-X.Y.Z-Linux-client.deb`

The client can now be run with

`SilentTanks-Client`

### Other users

The client is not packaged for .rpm or other package formats, self compilation and setup will be required. Follow the compilation and manual setup steps.

## Installation (server)

### Windows

Users should install windows subsystem for linux (WSL) and read the following section.

### Linux (debian based)

Open a terminal in the folder containing the packages.

Install the common package if not already installed

`sudo apt install ./silent-tanks-X.Y.Z-Linux-common.deb`

Install the server package

`sudo apt install ./silent-tanks-X.Y.Z-Linux-server.deb`

Ensure postgres is installed

`sudo apt install postgresql`

Now either generate a self signed .key and .crt file with

`sudo /usr/share/silent-tanks/setup/create-self-signed-cert.sh <YOUR_IP> <(optional) YOUR_DOMAIN>`

Or copy your existing files to /var/lib/silent-tanks/certs before continuing.

Next, setup the postgres database by calling the database setup script.

`sudo /usr/share/silent-tanks/setup/setup-database.sh`

You should now be able to start the server through the terminal

`sudo silent-tanks-server --port <YOUR_PORT> --address <YOUR_IP>`
