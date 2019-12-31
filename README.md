# FinServer
This application is a simple server that processes various requests sent by an application utilizing the `finapi` library.

## Compilation
Since it utilizes the `finapi` library itself, FinServer uses both a git clone and a cmake call to compile. All of this is wrapped up in the `init.sh` file which should be called once after cloning this repo. It also compiles the other library. Then the `compile.sh` file will compile the server.
