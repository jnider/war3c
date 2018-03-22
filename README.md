# war3c
An open-source version of an old game that start with war and ends with craft. 3.

Why I did this
--------------
It all started when I wanted to play a network game with my son, but our second computer is an ARM board with Ubuntu. I have run w3c
(original version) on Ubuntu on x86_64 (through Wine - those guys have really come a long way!) successfully, but this is taking it up a
notch (or two). I haven't actually tried running it on QEMU on Wine yet, but it sounds a bit wasteful. If you don't know what I'm talking
about, don't worry, it's not an important detail. The point is, I figured my nephew wants to learn to program and I can spend some time
helping out with the protocols, etc. so why not go the distance and re-write the whole thing.

This is based on the efforts of a lot of people who didn't even know they contributed since they were actually working on other projects.
Aura-bot is a w3c game server: https://github.com/Josko/aura-bot.git
StormLib is a library to read resource files: https://github.com/ladislav-zezula/StormLib.git
How to read maps: http://world-editor-tutorials.thehelper.net/cat_usersubmit.php?view=42787
The communications protocol: https://bnetdocs.org/packet/index

Dependencies
------------
In order to read maps and other resources, you must have the StormLib library (by Ladislav Zezula). It is available separately on github
(https://github.com/ladislav-zezula/StormLib.git) but integrated for convenience as a git sub-project. If you have never worked with
sub-projects before, you may want to read about them here: https://git-scm.com/book/en/v2/Git-Tools-Submodules. Basically when you clone
this repository, you will want to use "--recurse-submodules" to get everything at the same time:

git clone --recurse-submodules https://github.com/jnider/war3c.git

Contributions
-------------
If you would like to help, send me a message and tell me what you would like to do. If you don't have a preference, one will be provided
for you.

Future
------
It would be really cool if I could run this on my Android phone. That means a re-write in Java, and fighting with my wife about the best
UI, since without keyboard shortcuts battles would be frustrating at best. So that remains as future work for now, we have enough to do
with getting the "normal" stuff working.
