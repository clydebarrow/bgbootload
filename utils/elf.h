//
// Created by Clyde Stubbs on 4/4/17.
//

#ifndef UTILS_ELF_H
#define UTILS_ELF_H

typedef struct {
    uint8 magic[4];
    uint8 wordsize;        // 1 = 32bit, 2 = 64 bit
    uint8 endianness;      // 1 == little, 2 = big
    uint8 elfVer;          // 1
    uint8 system;
    uint8 abiversion;
    uint8 pad[7];
    uint8 type[2];
    uint8 isa[2];           // arm = 0x28
    uint8 elfVer2[4];
    uint8 entry[4];
    uint8 phoff[4];
    uint8 shoff[4];
    uint8 flags[4];
    uint8 ehsize[2];
    uint8 phentsize[2];
    uint8 phnum[2];
    uint8 shentsize[2];
    uint8 shnum[2];
    uint8 shstrndx[2];

} elf_file_header;

typedef struct {
    uint8 type[4];      // 1 = LOAD
    uint8 offset[4];
    uint8 vaddr[4];
    uint8 paddr[4];
    uint8 filesz[4];
    uint8 memsz[4];
    uint8 flags[4];
    uint8 align[4];
}   elf_program_header;
#endif //UTILS_ELF_H
