#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

int vc_size_lookup(const uint8_t input)
{
    switch (input) {
        case 0: return 0x8000;
        case 1: return 0x10000;
        case 2: return 0x20000;
        case 0xC: return 0x20000;
        case 4: return 0x40000;
        case 6: return 0x60000;
        case 8: return 0x80000;
        case 0x10: return 0x100000;
        case 0x20: return 0x200000;
        case 0x30: return 0x300000;
        case 0x40: return 0x400000;

        default: fprintf(stderr, "An unexpected error occured.\n"); exit(EXIT_FAILURE);
    }
}

int main(int argc, char** argv)
{
    if (argc != 4) {
        printf("Retroinject_C, a C version of the retroinject utility. Injects a rom file into an elf file.\n");
        printf("Syntax: ./retroinject path/to/input.elf path/to/input.sfc path/to/output.elf\n");
        exit(EXIT_SUCCESS);
    }

    FILE* input_elf = fopen(argv[1], "rb");
    if (!input_elf) {
        fprintf(stderr, "Error: Couldn't open input elf file.\n");
        exit(EXIT_FAILURE);
    }

    FILE* input_rom = fopen(argv[2], "rb");
    if (!input_rom) {
        fprintf(stderr, "Error: Couldn't open input rom file.\n");
        exit(EXIT_FAILURE);
    }

    // read in the elf file
    fseek(input_elf, 0, SEEK_END);
    long elf_length = ftell(input_elf);
    rewind(input_elf);

    uint8_t* elf_buffer = malloc(elf_length);
    assert(fread(elf_buffer, 1, elf_length, input_elf) == elf_length);
    fclose(input_elf);

    // read in the rom file
    fseek(input_rom, 0, SEEK_END);
    long rom_length = ftell(input_rom);
    rewind(input_rom);

    uint8_t* rom_buffer = malloc(rom_length);
    assert(fread(rom_buffer, 1, rom_length, input_rom) == rom_length);
    fclose(input_rom);

    int wup_index;
    for (wup_index = 0; wup_index < elf_length; wup_index += 4) {
        if (memcmp(&elf_buffer[wup_index], "\x57\x55\x50\x2D", 4) == 0) {
            break;
        }
    }

    if (wup_index == elf_length) {
        fprintf(stderr, "Error: Couldn't find WUP magic code.\n");
        exit(EXIT_FAILURE);
    }

    bool rom_is_nes = memcmp(rom_buffer, "\x4E\x45\x53\x1A", 4) == 0;
    uint8_t vc_magic[4];
    memcpy(vc_magic, &elf_buffer[wup_index + 16], 4);
    bool vc_is_nes;
    uint8_t vc_size_key;
    int vc_inject;
    if (memcmp(vc_magic, "\x4E\x45\x53\x1A", 4) == 0 || memcmp(vc_magic, "\x4E\x45\x53\x00", 4) == 0) {
        vc_is_nes = true;
        vc_size_key = elf_buffer[wup_index - 14];
        vc_inject = wup_index + 16;
    } else {
        vc_is_nes = false;
        vc_size_key = elf_buffer[wup_index - 22];
        vc_inject = wup_index + 12;
    }
    int vc_size = vc_size_lookup(vc_size_key);
    if (vc_is_nes)
        vc_size += 16;
    if (rom_length > vc_size) {
        fprintf(stderr, "Error: Rom is too large to be injected.\n");
        exit(EXIT_FAILURE);
    }
    if (rom_is_nes != vc_is_nes) {
        fprintf(stderr, "Error: Elf and rom need to be of the same type ((S)NES).\n");
        exit(EXIT_FAILURE);
    }

    FILE* output_elf = fopen(argv[3], "wb");
    if (!output_elf) {
        fprintf(stderr, "Error: Couldn't open output file.\n");
        exit(EXIT_FAILURE);
    }

    fwrite(elf_buffer, 1, vc_inject, output_elf);
    fwrite(rom_buffer, 1, rom_length, output_elf);
    fwrite(&elf_buffer[vc_inject + rom_length], 1, elf_length - vc_inject - rom_length, output_elf);
    fclose(output_elf);
    free(elf_buffer);
    free(rom_buffer);
}
