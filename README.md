# installation
```
g++ installer.cpp -o install # compile installer (in case you need it)
```

```
./install # install builder
```

```
./install reinstall # reinstall builder 
```

```
belder uninstall #uninstall builder
```
# builder
you can launch builder from any folder

to launch builder type "belder"

to specify starting file type the file name right after "belder", in other case builder will look for main.cpp or main.c

builder only works with files with .h, .hpp, .c, .cpp extensions, the builder does not see files with other extensions
```
belder file.cpp  # starting file - file.cpp
belder #builder will look for main.cpp or main.c
```
the builder remembers the latest settings and the entered flags, so if you just enter a command without flags or arguments, the build will go the same way as the last time

if you write a "config" right after the "belder" and add flags, then you will simply update the settings without building. Next time, without arguments, the build will go through with these settings.
``` 
belder config --no-link-force file.cpp -lmyLib -o out # just updating config
```
You can type "run" in any place to run program after build
```
belder run -o out # will run a program
```
Type "belder status" to see all the current settings for a project
```
belder status # will type all current settings
```
To uninstall builder simply type
```
belder uninstall
```
# creating library
If the name of the output file matches the standards of the static library name, you will get the library at the output (.a)

Builder cannot make other libraries

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

-x86 compile for x86 (default setting)

-riscv compile for riscv

-I[path] additionally, specify the directory in which to search for headers and source files. This flag is required only for directories located outside the project directory. The specified directory will be treated in the same way as the main project directory, builder will find all headers and source files inside all subdirectories of the specified directory. You must specify the full path to the new directory

--no-include [folder] remove folder added by the -I flag from config file