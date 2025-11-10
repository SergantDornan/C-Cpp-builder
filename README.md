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

Builder is a full-fledged build system, which is designed to replace Make and CMake. The program finds all files in the project subfolders, compiles them, analyzes object files, analyzes found libraries and decides what ultimately needs to be linked. The set of files for analysis can be manipulated using various flags, which are listed below.

you can launch builder from any folder

to launch builder type "belder"

to specify starting file type the file name right after "belder", in other case builder will look for main.cpp or main.c

builder only works with libraries and files with .h, .hpp, .c, .cpp, .s, .S, .asm extensions, the builder does not see files with other extensions

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
Type "belder help" to get list of instructions and flags
```
belder help
```
You can type "run" in any place to run program after build
```
belder run -o out # will run a program
```
Type "belder status" to see all the current settings for a project
```
belder status # will type all current settings
```
Type "clean","mrproper" or "clear" to remove folder with object and dep files for your project
```
belder clean
belder mrproper
belder clear
```

To uninstall builder simply type:
```
belder uninstall
```
To recompile belder itself type:
```
belder reinstall # only works if you still got original source code
```

# creating library
If the name of the output file matches the standards of the static or shared library name, you will get the library at the output

```
belder start.cpp -o libMylib.a # will create a static library
belder start.cpp -o libMylib.so # will create a shared library
```

# Pocketbuilder

If you install pocketbuilder with command

```
make pocket
```

Then you will get a binary file "pocketbuilder". You can copy this binary file to any folder. Run this file from this folder to build the project located in this folder. Pocketbuilder will create a "builder" folder containing object and dependency files inside the project, rather than in the HOME directory.



# flags
-log showing log information while building

--rebuild, -reb recompiling all files in project

--relink, -rel relinking all files in project

-o [filename] specify output file name

--no-link-force [file] [file] [file] specify files that definitely will not be linked
```
belder --no-link-force file1.cpp file2.cpp libMylib.a
belder --no-link-force Mylib # "Mylib" will be considered as a library, so the two files will not be linked: libMylib.a, libMylib.so
```

--link-force [file] [file] [file] specify files that definitely will be linked
```
belder --link-force file1.cpp file2.cpp libMylib.a
belder --link-force Mylib # "Mylib" will be considered as a library. If belder finds both libMylib.a and libMylib.so then libMylib.so  will be linked
```

--default-link [file] [file] [file] let belder decide whether to link files or not
```
belder --default-link file1.cpp file2.cpp libMylib.a (exmp)
belder --default-link Mylib # "Mylib" will be considered as a library, so the two files will be excluded from the force-link and force-unlink lists: libMylib.a, libMylib.so
```

-l[short lib name] link libraries with -l flag and short library name
```
belder -lMylib # is equivalent to "belder --link-force Mylib"
```

--CC/CXX [compiler] Specify compiler. Type "default" to use standart gcc and g++ compiler

```
belder config --CC /dir1/dir2/compiler # use custom compiler for C language
belder config --CXX /dir1/dir2/compiler # use custom compiler for C++ language
belder config --CC default # use standart gcc for C language
belder config --CXX default # use standart g++ for C++ language
```

-I[path] additionally, specify the directory in which to search for headers and source files. This flag is required only for directories located outside the project directory. The specified directory will be treated in the same way as the main project directory, builder will find all headers and source files inside all subdirectories of the specified directory. You must specify the full path to the new directory

--no-include [path] [path] [path] specify the directories in which NOT to search for headers, source files and libraries

--defult-include [path] [path] [path] specify the directories to exclude from --no-include and additional include list

--compile-flags [many flags] Compilation will occur with these flags, but not linking

--link-flags [many flags]  Linking will occur with these flags, but not compilation

--clear-flags, --clean-flags, --flags-clean, --flags-clear Delete all flags from config file

--clear-options, --clean-options Reset force-link, force-unlink, additional directories, no-include directories lists

--idgaf Ignore multiple definition error

Any flag not specified above will be considered as a flag to the compiler