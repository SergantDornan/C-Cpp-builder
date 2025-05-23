# installation
```
make install #install builder
```

```
make pocket # build pocketbuilder
```

```
belder uninstall #uninstall builder
```

```
belder reinstall #reinstall builder
```
# builder
you can launch builder from any folder

to launch builder type "belder"

to specify starting file type the file name right after "belder", in other case builder will look for main.cpp or main.c

builder only works with files with .h, .hpp, .c, .cpp, .s, .S, .asm extensions, the builder does not see files with other extensions

**WARNING**
The belder does not work if your assembly files contain not C-style comments


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
Type "clean" or "mrproper" to remove folder with assembly, object and dep files for your project
```
belder clean
belder mrproper
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

# Pocketbuilder

If you install pocketbuilder with command

```
make pocket
```

Then you will get a binary file "pocketbuilder". You can copy this binary file to any folder. Run this file from this folder to build the project located in this folder. Pocketbuilder will create a "builder" folder containing assembler files, object files, and dependency files inside the project, rather than in the home folder.



# flags
-log showing log information while building

--rebuild / -reb recompiling all files in project

--relink / -rel relinking all files in project

-o [filename] specify output file name

-l[libName] link specific lib

--no-link-lib [libName] unlink lib (remove it from config file)

--no-link-force [file_name/folder_name] unlink file or folder with force

--link-force [filename] link file with force

--default-link [filename] let builder decide how to link this file (remove file from force link or force unlink list)

--CC/CXX [compiler] Specify compiler. Type "default" to use standart gcc and g++ compiler

```
belder config --CC /dir1/dir2/compiler # use custom compiler for C language
belder config --CXX /dir1/dir2/compiler # use custom compiler for C++ language
belder config --CC default # use standart gcc for C language
belder config --CXX default # use standart g++ for C++ language
```

--preproc [preprocessor] Specify preprocessor

-I[path] additionally, specify the directory in which to search for headers and source files. This flag is required only for directories located outside the project directory. The specified directory will be treated in the same way as the main project directory, builder will find all headers and source files inside all subdirectories of the specified directory. You must specify the full path to the new directory

--no-include [folder] remove folder added by the -I flag from config file

--compile-flags [some flags] Compilation will occur with these flags, but not linking

--link-flags [some flags]  Linking will occur with these flags, but not compilation

--clear-flags Delete all flags from config file

--idgaf Ignore multiple definition

Any flag not specified above will be considered as a flag to the compiler