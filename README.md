# installation
```
g++ installer.cpp -o install # build installer
```

```
./install # install builder
```

```
./install reinstall # reinstall builder 
```
# builder
you can launch builder from any folder

to launch builder type "belder"

to specify starting file type the file name right after "belder", in other case builder will look for main.cpp or main.c

builder works only with files with .h, .hpp, .c, .cpp extensions, in other case builder will not see them 
```
belder file.cpp  # starting file - file.cpp
belder #builder will look for main.cpp or main.c
```
the builder remembers the latest settings and the entered flags, so if you just enter a command without flags and arguments, the build will go the same way as the last time

if you write a "config" right after the "belder" and add flags, then you will simply update the settings without building. Next time, without arguments, the build will go through with these settings.
``` 
belder config --static -o out # just updating configs
```
You can type "run" in any place to run program after build
```
belder run -o out # will run a program
```
Type belder status to see all the current settings for a project
```
belder status # will type all current settings
```
To uninstall builder simply type
```
belder uninstall
```
# creating library
If the name of the output file matches the standards of the static library name, you will get the library at the output (.a)

Builder cannot make .so library

```
belder start.cpp -o libMylib.a # will create a static library
```
# flags
-log showing log information while building

--rebuild / -reb rebuilding all configs and recompiling all files in projects

-o [filename] specify output file name

-l[libName] link specific lib

--no-link-lib [libName] unlink lib (remove it from config file)

--no-link-force [filename] unlink file with force

--link-force [filename] link file with force

--default-link [filename] let builder decide how to link this file (remove file from force link or force unlink list)