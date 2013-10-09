Building client on linux (tested in ubuntu)

Build a static version of the allegro library
http://ventilatorxor.wordpress.com/2011/08/07/linux-allegro5-static-linking-for-beginners/

The info below is outdated. The latest info is on the github wiki.

BoostPro Installer options

-multithreaded
-multithreaded debug


old installation instructions

client
______

install the boost library
install openssl using the old windows installer

Add the following to Linker -> Input -> Additional Dependencies:
libeay32.lib
ssleay32.lib


server
______

install the boost library
install openssl on ubuntu (should be easy)
