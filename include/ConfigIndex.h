#ifndef BELDER_CONFIG_INDEX_H
#define BELDER_CONFIG_INDEX_H

// Индексы полей в векторе parameters (project config).
// Эти константы описывают раскладку config-файла проекта,
// подставляйте их вместо магических чисел в parameters[].

#define CFG_ENTRY             0   // main input (путь к стартовому файлу)
#define CFG_OUTPUT            1   // output name
#define CFG_FORCE_LINK_LIBS   2   // libs linking (force link libs)
#define CFG_FORCE_LINK        3   // force link list
#define CFG_FORCE_UNLINK      4   // force unlink list
#define CFG_COMPILERS         5   // compilers (C, C++)
#define CFG_ADD_INCLUDE       6   // additional -I list
#define CFG_CXX_STANDARD      7   // C++ standart
#define CFG_OPT               8   // optimization flag
#define CFG_DEBUG             9   // debug flag
#define CFG_COMPILE_FLAGS    10   // flags to compiler
#define CFG_LINK_FLAGS       11   // flags to linker
#define CFG_GENERAL_FLAGS    12   // general flags
#define CFG_FORCE_UNLINK_LIBS 13  // force unlink libs
#define CFG_FORCE_UNLINK_DIRS 14  // force unlink dirs
#define CFG_C_STANDARD       15   // C standart

#define CFG_PROFILE          16

#define CFG_COUNT            17   // общее количество полей в config

#endif // BELDER_CONFIG_INDEX_H
