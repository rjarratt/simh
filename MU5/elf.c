#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "elf.h"

#define MAX_SYMBOLS 1024
#define MAX_RELOCATIONS 10240
#define MAX_STRING_DATA 10240
#define MAX_SECTIONS 32
#define MAX_PROGRAM_HEADERS 32

typedef struct elf32_section
{
    Elf32_Shdr section_header;
    Elf32_Half section_index;
    struct elf32_section *related_section;
    void *data;
    void *(*encode_data_entry)(void *);
} Elf32_Section;

typedef struct elf32_context
{
    Elf32_Ehdr elf_header;
    Elf32_Section *symbol_table_section;
    Elf32_Section *section_header_string_table_section;
    Elf32_Section *string_table_section;
    Elf32_Section section_table[MAX_SECTIONS];
    Elf32_Phdr program_header_table[MAX_PROGRAM_HEADERS];
} Elf32_Context;

static Elf32_Section *add_section(Elf32_Context *ctx, Elf32_Shdr *header);
static int is_empty_section(Elf32_Section *section);
static void create_empty_section(Elf32_Context *ctx);
static void create_symbol_table(Elf32_Context *ctx);
static void create_section_header_string_table(Elf32_Context *ctx);
static void create_string_table(Elf32_Context *ctx);
static void create_relocation_table(Elf32_Context *ctx, Elf32_Section *applied_section);
static int is_section_loadable(Elf32_Section *section);
static Elf32_Off calculate_section_offset(Elf32_Context *ctx, Elf32_Section *section);

static int add_string(Elf32_Section *section, char *string);
static char *get_string(Elf32_Section *section, int string_index);

static Elf32_Sym *get_linker_symbol(Elf32_Section *section, int string_index);
static int get_symbol_index(Elf32_Section *section, Elf32_Sym *sym);
static Elf32_Rela *get_rela(Elf32_Section *section, int rela_index);

static Elf32_Ehdr *encode_ehdr(Elf32_Ehdr *header);
static Elf32_Shdr *encode_shdr(Elf32_Shdr *header);
static Elf32_Phdr *encode_phdr(Elf32_Phdr *header);
static Elf32_Sym *encode_sym(Elf32_Sym *sym);
static Elf32_Rela *encode_rela(Elf32_Rela *rela);
static Elf32_Half encode_half(Elf32_Half half);
static Elf32_Word encode_word(Elf32_Word word);
static Elf32_Sword encode_sword(Elf32_Sword word);
static Elf32_Off encode_off(Elf32_Off off);
static Elf32_Addr encode_addr(Elf32_Addr addr);

static void decode_ehdr(Elf32_Ehdr *header);
static void decode_shdr(Elf32_Shdr *header);
static void decode_phdr(Elf32_Phdr *header);
static void decode_sym(Elf32_Sym *sym);
static void decode_rela(Elf32_Rela *rela);
static Elf32_Half decode_half(Elf32_Half half);
static Elf32_Word decode_word(Elf32_Word word);
static Elf32_Sword decode_sword(Elf32_Sword word);
static Elf32_Off decode_off(Elf32_Off off);
static Elf32_Addr decode_addr(Elf32_Addr addr);

void *elf_new_file(Elf32_Half e_type, Elf32_Half e_machine, Elf32_Word e_flags)
{
    Elf32_Context *ctx = (Elf32_Context *)malloc(sizeof(Elf32_Context));
    memset(ctx, 0, sizeof(Elf32_Context));
    ctx->elf_header.e_ident[EI_MAG0] = ELFMAG0;
    ctx->elf_header.e_ident[EI_MAG1] = ELFMAG1;
    ctx->elf_header.e_ident[EI_MAG2] = ELFMAG2;
    ctx->elf_header.e_ident[EI_MAG3] = ELFMAG3;
    ctx->elf_header.e_ident[EI_CLASS] = ELFCLASS32;
    ctx->elf_header.e_ident[EI_DATA] = ELFDATA2MSB; /* big endian */
    ctx->elf_header.e_ident[EI_VERSION] = EV_CURRENT;

    ctx->elf_header.e_type = e_type;
    ctx->elf_header.e_machine = 0;
    ctx->elf_header.e_version = EV_CURRENT;
    ctx->elf_header.e_entry = 0;
    ctx->elf_header.e_flags = e_flags;
    ctx->elf_header.e_ehsize = sizeof(ctx->elf_header);

    ctx->elf_header.e_phentsize = sizeof(Elf32_Phdr);
    ctx->elf_header.e_shentsize = sizeof(Elf32_Shdr);

    create_empty_section(ctx);
    create_section_header_string_table(ctx);
    create_string_table(ctx);
    create_symbol_table(ctx);

    return ctx;
}

void elf_set_entry(void *context, Elf32_Addr e_entry)
{
    Elf32_Context *ctx = context;
    ctx->elf_header.e_entry = e_entry;
}

int elf_add_code_section(void *context, Elf32_Word word_size, Elf32_Addr address, char *data)
{
    Elf32_Context *ctx = context;
    Elf32_Shdr header;
    Elf32_Section *section;
    memset(&header, 0, sizeof(Elf32_Shdr));
    header.sh_name = add_string(ctx->section_header_string_table_section, ".text");
    header.sh_type = SHT_PROGBITS;
    header.sh_flags = SHF_ALLOC | SHF_EXECINSTR;
    header.sh_addr = address;
    header.sh_entsize = word_size;
    section = add_section(ctx, &header);
    section->data = data;
    create_relocation_table(ctx, section);
    return section->section_index;
}

int elf_add_data_section(void *context, Elf32_Word word_size, Elf32_Addr address, char *data)
{
    Elf32_Context *ctx = context;
    Elf32_Shdr header;
    Elf32_Section *section;
    memset(&header, 0, sizeof(Elf32_Shdr));
    header.sh_name = add_string(ctx->section_header_string_table_section, ".data");
    header.sh_type = SHT_PROGBITS;
    header.sh_flags = SHF_ALLOC | SHF_WRITE;
    header.sh_addr = address;
    header.sh_entsize = word_size;
    section = add_section(ctx, &header);
    section->data = data;
    return section->section_index;
}

int elf_add_bss_section(void *context, Elf32_Word word_size, Elf32_Addr address)
{
    Elf32_Context *ctx = context;
    Elf32_Shdr header;
    Elf32_Section *section;
    memset(&header, 0, sizeof(Elf32_Shdr));
    header.sh_name = add_string(ctx->section_header_string_table_section, ".bss");
    header.sh_type = SHT_NOBITS;
    header.sh_flags = SHF_ALLOC | SHF_WRITE;
    header.sh_addr = address;
    header.sh_entsize = word_size;
    section = add_section(ctx, &header);
    return section->section_index;
}

void elf_update_section_address(void *context, Elf32_Half section_index, Elf32_Addr address)
{
    Elf32_Context *ctx = context;
    Elf32_Section *section = &ctx->section_table[section_index];

    section->section_header.sh_addr = address;
}

void elf_update_section_size(void *context, Elf32_Half section_index, Elf32_Word size)
{
    Elf32_Context *ctx = context;
    Elf32_Section *section = &ctx->section_table[section_index];

    section->section_header.sh_size = size;
}

int elf_get_relocation_section(void *context, Elf32_Half section_index)
{
    int result = -1;
    Elf32_Context *ctx = context;
    Elf32_Section *section = &ctx->section_table[section_index];
    if (section->related_section != NULL)
    {
        result = section->related_section->section_index;
    }

    return result;
}

void elf_add_relocation_entry(void *context, Elf32_Half code_section_index, Elf32_Addr offset, void *symbol, int relocation_type, Elf32_Sword addend)
{
    Elf32_Context *ctx = context;
    Elf32_Section *code_section;
    Elf32_Section *rela_section;
    Elf32_Sym *sym = symbol;
    int num_relas;

    code_section = &ctx->section_table[code_section_index];
    rela_section = code_section->related_section;

    num_relas = rela_section->section_header.sh_size / sizeof(Elf32_Rela);
    if (num_relas >= MAX_RELOCATIONS)
    {
        perror("Too many relocations");
        exit(1);
    }

    Elf32_Rela *rela = (Elf32_Rela *)rela_section->data + num_relas;
    rela->r_offset = offset;
    rela->r_info = ELF32_R_INFO(get_symbol_index(ctx->symbol_table_section, sym), relocation_type);
    rela->r_addend = addend;
    rela_section->section_header.sh_size += sizeof(Elf32_Rela);
}

void *elf_add_global_symbol(void *context, char *name, Elf32_Addr value, Elf32_Word size, int type, Elf32_Half section_index)
{
    Elf32_Context *ctx = context;
    int num_symbols = ctx->symbol_table_section->section_header.sh_size / sizeof(Elf32_Sym);
    if (num_symbols >= MAX_SYMBOLS)
    {
        perror("Too many symbols");
        exit(1);
    }

    Elf32_Sym *sym = (Elf32_Sym *)ctx->symbol_table_section->data + num_symbols;
    sym->st_name = add_string(ctx->string_table_section, name);
    sym->st_value = value;
    sym->st_size = size;
    sym->st_info = ELF32_ST_INFO(STB_GLOBAL, type);
    sym->st_other = 0;
    sym->st_shndx = section_index;
    ctx->symbol_table_section->section_header.sh_size += sizeof(Elf32_Sym);
    return sym;
}

void elf_update_symbol(void *symbol, Elf32_Addr value)
{
    Elf32_Sym *sym = symbol;
    sym->st_value = value;
}

void elf_add_binary_data_to_section(void *context, Elf32_Half section_index, char *data, int length)
{
    Elf32_Context *ctx = context;
    Elf32_Section *section = &ctx->section_table[section_index];
    if (section->section_header.sh_type != SHT_NOBITS)
    {
        memcpy((char *)section->data + section->section_header.sh_size, data, length);
    }
    section->section_header.sh_size += length;
}

void elf_write_file(void *context, char *file_name)
{
    int num_sections;
    Elf32_Context *ctx = context;
    int section_index;
    FILE *f = fopen(file_name, "wb");

    num_sections = ctx->elf_header.e_shnum;

    ctx->elf_header.e_phoff = (ctx->elf_header.e_type == ET_EXEC) ? sizeof(ctx->elf_header) : 0;
    ctx->elf_header.e_shoff = sizeof(ctx->elf_header) + ctx->elf_header.e_phentsize * ctx->elf_header.e_phnum;

    fwrite(encode_ehdr(&ctx->elf_header), sizeof(ctx->elf_header), 1, f);

    /* write program header table (if required) */
    if (ctx->elf_header.e_type == ET_EXEC)
    {
        for (section_index = 0; section_index < num_sections; section_index++)
        {
            Elf32_Section *section = &ctx->section_table[section_index];
            if (is_section_loadable(section))
            {
                Elf32_Phdr p_header;
                p_header.p_type = PT_LOAD;
                p_header.p_offset = (section->section_header.sh_type == SHT_NOBITS) ? 0 : calculate_section_offset(ctx, section);
                p_header.p_vaddr = section->section_header.sh_addr;
                p_header.p_paddr = section->section_header.sh_addr;
                p_header.p_filesz = (section->section_header.sh_type == SHT_NOBITS) ? 0 : section->section_header.sh_size;
                p_header.p_memsz = section->section_header.sh_size;
                p_header.p_align = 0;

                p_header.p_flags = PF_R;

                if ((section->section_header.sh_flags & SHF_EXECINSTR) == SHF_EXECINSTR)
                {
                    p_header.p_flags |= PF_X;
                }

                if ((section->section_header.sh_flags & SHF_WRITE) == SHF_WRITE)
                {
                    p_header.p_flags |= PF_W;
                }

                fwrite(encode_phdr(&p_header), sizeof(Elf32_Phdr), 1, f);
            }
        }
    }

    /* write section table */
    for (section_index = 0; section_index < num_sections; section_index++)
    {
        Elf32_Section *section = &ctx->section_table[section_index];
        if (!is_empty_section(section))
        {
            section->section_header.sh_offset = calculate_section_offset(ctx, section);
        }

        section->section_header.sh_size = section->section_header.sh_size;
        fwrite(encode_shdr(&section->section_header), sizeof(Elf32_Shdr), 1, f);
    }

    /* write sections */
    for (section_index = 0; section_index < num_sections; section_index++)
    {
        Elf32_Section *section = &ctx->section_table[section_index];
        int section_size = section->section_header.sh_size;
        int entry_size = section->section_header.sh_entsize;
        if (section->section_header.sh_type != SHT_NOBITS)
        {
            if (section->encode_data_entry == NULL)
            {
                fwrite(section->data, 1, section_size, f);
            }
            else
            {
                int i;
                int n = section_size / entry_size;
                for (i = 0; i < n; i++)
                {
                    void *buffer = section->encode_data_entry((char *)(section->data) + (i * entry_size));
                    fwrite(buffer, entry_size, 1, f);
                }
            }
        }
    }

    fclose(f);
}

void *elf_read_file(FILE *f, int check_is_elf)
{
    char *error = NULL;
    Elf32_Context *ctx = (Elf32_Context *)malloc(sizeof(Elf32_Context));
    memset(ctx, 0, sizeof(Elf32_Context));
    int i;

    if (fread(&ctx->elf_header, sizeof(Elf32_Ehdr), 1, f) != 1)
    {
        if (!check_is_elf)
        {
            error = "Cannot read ELF header";
        }
    }

    decode_ehdr(&ctx->elf_header);

    if (!error && (ctx->elf_header.e_ident[EI_MAG0] != ELFMAG0 || ctx->elf_header.e_ident[EI_MAG1] != ELFMAG1 || ctx->elf_header.e_ident[EI_MAG2] != ELFMAG2 || ctx->elf_header.e_ident[EI_MAG3] != ELFMAG3))
    {
        if (!check_is_elf)
        {
            error = "Not a valid ELF file - missing file identification";
        }
    }

    if (!error)
    {
        if (ctx->elf_header.e_ident[EI_CLASS] != ELFCLASS32)
        {
            perror("Not a 32-bit ELF file");
            exit(1);
        }

        if (ctx->elf_header.e_phnum > MAX_PROGRAM_HEADERS)
        {
            perror("Too many program headers");
            exit(1);
        }

        if (ctx->elf_header.e_shnum > MAX_SECTIONS)
        {
            perror("Too many sections");
            exit(1);
        }

        if (ctx->elf_header.e_phoff != 0)
        {
            fseek(f, ctx->elf_header.e_phoff, SEEK_SET);
            if (fread(ctx->program_header_table, sizeof(Elf32_Phdr), ctx->elf_header.e_phnum, f) != ctx->elf_header.e_phnum)
            {
                perror("Could not read the program headers");
                exit(1);
            }

            for (i = 0; i < ctx->elf_header.e_phnum; i++)
            {
                decode_phdr(&ctx->program_header_table[i]);
            }
        }

        fseek(f, ctx->elf_header.e_shoff, SEEK_SET);
        for (i = 0; i < ctx->elf_header.e_shnum; i++)
        {
            Elf32_Section *section = &ctx->section_table[i];
            memset(section, 0, sizeof(Elf32_Section));
            section->section_index = i;
            if (fread(&section->section_header, sizeof(Elf32_Shdr), 1, f) != 1)
            {
                perror("Could not read section header");
                exit(1);
            }
            decode_shdr(&section->section_header);
        }

        for (i = 0; i < ctx->elf_header.e_shnum; i++)
        {
            Elf32_Section *section = &ctx->section_table[i];
            if (section->section_header.sh_type != SHT_NULL && section->section_header.sh_type != SHT_NOBITS)
            {
                if (section->section_header.sh_size != 0)
                {
                    section->data = malloc(section->section_header.sh_size);
                    fseek(f, section->section_header.sh_offset, SEEK_SET);
                    if (fread(section->data, section->section_header.sh_size, 1, f) != 1)
                    {
                        perror("Could not read section data");
                        exit(1);
                    }

                    if (section->section_header.sh_type == SHT_SYMTAB)
                    {
                        int sym_index;
                        int num_syms = section->section_header.sh_size / section->section_header.sh_entsize;
                        ctx->symbol_table_section = section;
                        for (sym_index = 0; sym_index < num_syms; sym_index++)
                        {
                            decode_sym(get_linker_symbol(section, sym_index));
                        }
                    }
                    else if (section->section_header.sh_type == SHT_STRTAB)
                    {
                        if (i == ctx->elf_header.e_shstrndx)
                        {
                            ctx->section_header_string_table_section = section;
                        }
                        else
                        {
                            ctx->string_table_section = section;
                        }
                    }
                    else if (section->section_header.sh_type == SHT_RELA)
                    {
                        int rela_index;
                        int num_relas = section->section_header.sh_size / section->section_header.sh_entsize;
                        for (rela_index = 0; rela_index < num_relas; rela_index++)
                        {
                            decode_rela(get_rela(section, rela_index));
                        }

                        ctx->section_table[section->section_header.sh_info].related_section = section;
                    }
                }
            }
        }
    }
    else if (!check_is_elf)
    {
        perror(error);
        exit(1);
    }
    else
    {
        free(ctx);
        ctx = NULL;
    }

    return ctx;
}

void elf_get_elf_header(void *context, Elf32_Ehdr *header)
{
    Elf32_Context *ctx = context;
    memcpy(header, &ctx->elf_header, sizeof(Elf32_Ehdr));
}

void elf_get_program_header(void *context, Elf32_Phdr *header, int header_index)
{
    Elf32_Context *ctx = context;
    memcpy(header, &ctx->program_header_table[header_index], sizeof(Elf32_Phdr));
}

void elf_get_section_header(void *context, Elf32_Shdr *header, char **data, int section_index)
{
    Elf32_Context *ctx = context;
    memcpy(header, &ctx->section_table[section_index].section_header, sizeof(Elf32_Shdr));
    if (data != NULL)
    {
        *data = ctx->section_table[section_index].data;
    }
}

void elf_get_symbol(void *context, Elf32_Sym *symbol, int symbol_index)
{
    Elf32_Context *ctx = context;
    memcpy(symbol, get_linker_symbol(ctx->symbol_table_section, symbol_index), sizeof(Elf32_Sym));
}

char *elf_get_string(void *context, int string_index)
{
    char *result;
    Elf32_Context *ctx = context;
    result = get_string(ctx->string_table_section, string_index);
    return result;
}

char *elf_get_section_name(void *context, int string_index)
{
    char *result;
    Elf32_Context *ctx = context;
    result = get_string(ctx->section_header_string_table_section, string_index);
    return result;
}

void elf_process_defined_symbols(void *context, void *(*process_symbol)(char *name, Elf32_Addr value, Elf32_Word size, int type, unsigned char st_other, Elf32_Half section_index))
{
    int sym_index;
    int num_syms;
    Elf32_Context *ctx = context;
    num_syms = ctx->symbol_table_section->section_header.sh_size / ctx->symbol_table_section->section_header.sh_entsize;
    for (sym_index = 1; sym_index < num_syms; sym_index++)
    {
        Elf32_Sym *sym = get_linker_symbol(ctx->symbol_table_section, sym_index);
        if (sym->st_shndx != SHN_UNDEF)
        {
            process_symbol(get_string(ctx->string_table_section, sym->st_name), sym->st_value, sym->st_size, ELF_ST_TYPE(sym->st_info), sym->st_other, sym->st_shndx);
        }
    }
}


static Elf32_Section *add_section(Elf32_Context *ctx, Elf32_Shdr *header)
{
    ctx->elf_header.e_shnum++;
    if (ctx->elf_header.e_shnum >= MAX_SECTIONS)
    {
        perror("Too many sections");
        exit(1);
    }

    Elf32_Section *new_section = &ctx->section_table[ctx->elf_header.e_shnum - 1];
    memcpy(&new_section->section_header, header, sizeof(Elf32_Shdr));
    new_section->data = NULL;
    new_section->section_index = ctx->elf_header.e_shnum - 1;
    new_section->encode_data_entry = NULL;

    if (ctx->elf_header.e_type == ET_EXEC && is_section_loadable(new_section))
    {
        ctx->elf_header.e_phnum++;
    }

    return new_section;
}

static int is_empty_section(Elf32_Section *section)
{
    return section->section_header.sh_type == SHT_NULL;
}

static void create_empty_section(Elf32_Context *ctx)
{
    Elf32_Shdr header;
    memset(&header, 0, sizeof(Elf32_Shdr));
    header.sh_name = 0;
    header.sh_type = SHT_NULL;
    header.sh_link = SHN_UNDEF;
    add_section(ctx, &header);
}

static void create_symbol_table(Elf32_Context *ctx)
{
    Elf32_Shdr header;
    memset(&header, 0, sizeof(Elf32_Shdr));
    header.sh_name = add_string(ctx->section_header_string_table_section, ".symtab");
    header.sh_type = SHT_SYMTAB;
    header.sh_flags = 0;
    header.sh_addr = 0;
    header.sh_link = ctx->string_table_section->section_index;
    header.sh_entsize = sizeof(Elf32_Sym);
    ctx->symbol_table_section = add_section(ctx, &header);
    ctx->symbol_table_section->data = calloc(MAX_SYMBOLS, sizeof(Elf32_Sym));
    ctx->symbol_table_section->encode_data_entry = encode_sym;

    Elf32_Sym *empty_sym = ctx->symbol_table_section->data;
    empty_sym->st_shndx = encode_half(SHN_UNDEF);
    ctx->symbol_table_section->section_header.sh_size = sizeof(Elf32_Sym);
}

static void create_section_header_string_table(Elf32_Context *ctx)
{
    Elf32_Shdr header;
    memset(&header, 0, sizeof(Elf32_Shdr));
    header.sh_name = 1;
    header.sh_type = SHT_STRTAB;
    header.sh_flags = 0;
    header.sh_addr = 0;
    header.sh_entsize = 0;
    ctx->section_header_string_table_section = add_section(ctx, &header);
    ctx->section_header_string_table_section->data = calloc(1, MAX_STRING_DATA);
    ctx->section_header_string_table_section->section_header.sh_size = 1;
    ctx->elf_header.e_shstrndx = ctx->section_header_string_table_section->section_index;

    add_string(ctx->section_header_string_table_section, ".shstrtab");
}

static void create_string_table(Elf32_Context *ctx)
{
    Elf32_Shdr header;
    memset(&header, 0, sizeof(Elf32_Shdr));
    header.sh_name = add_string(ctx->section_header_string_table_section, ".strtab");
    header.sh_type = SHT_STRTAB;
    header.sh_flags = 0;
    header.sh_addr = 0;
    header.sh_entsize = 0;
    ctx->string_table_section = add_section(ctx, &header);
    ctx->string_table_section->data = calloc(1, MAX_STRING_DATA);
    ctx->string_table_section->section_header.sh_size = 1;
}

static void create_relocation_table(Elf32_Context *ctx, Elf32_Section *applied_section)
{
    Elf32_Section *section;
    Elf32_Shdr header;
    memset(&header, 0, sizeof(Elf32_Shdr));
    header.sh_name = add_string(ctx->section_header_string_table_section, ".rela.text");
    header.sh_type = SHT_RELA;
    header.sh_link = ctx->symbol_table_section->section_index;
    header.sh_info = applied_section->section_index;
    header.sh_flags = 0;
    header.sh_addr = 0;
    header.sh_entsize = sizeof(Elf32_Rela);
    section = add_section(ctx, &header);
    section->data = calloc(sizeof(Elf32_Rela), MAX_RELOCATIONS);
    section->encode_data_entry = encode_rela;
    applied_section->related_section = section;
}

static int is_section_loadable(Elf32_Section *section)
{
    int result = ((section->section_header.sh_flags & SHF_ALLOC) == SHF_ALLOC);
    return result;
}

static Elf32_Off calculate_section_offset(Elf32_Context *ctx, Elf32_Section *section)
{
    int section_index;
    Elf32_Off result = sizeof(ctx->elf_header) + ctx->elf_header.e_phentsize * ctx->elf_header.e_phnum + ctx->elf_header.e_shentsize * ctx->elf_header.e_shnum;
    for (section_index = 0; section_index < section->section_index; section_index++)
    {
        result += ctx->section_table[section_index].section_header.sh_size;
    }

    return result;
}

static int add_string(Elf32_Section *section, char *string)
{
    int result = section->section_header.sh_size;
    int len = strlen(string) + 1;
    char *ptr = section->data;

    if ((section->section_header.sh_size + len) > MAX_STRING_DATA)
    {
        perror("Too much string data");
        exit(1);
    }

    strcpy(&ptr[result], string);
    section->section_header.sh_size += len;

    return result;
}

static char *get_string(Elf32_Section *section, int string_index)
{
    char *ptr = (char *)(section->data) + string_index;
    return ptr;
}

static Elf32_Sym *get_linker_symbol(Elf32_Section *section, int symbol_index)
{
    Elf32_Sym *result = (Elf32_Sym *)((char *)section->data + symbol_index * section->section_header.sh_entsize);
    return result;
}

static int get_symbol_index(Elf32_Section *section, Elf32_Sym *sym)
{
    int result = sym - (Elf32_Sym *)section->data;
    return result;
}


static Elf32_Rela *get_rela(Elf32_Section *section, int rela_index)
{
    Elf32_Rela *result = (Elf32_Rela *)((char *)section->data + rela_index * section->section_header.sh_entsize);
    return result;
}

static Elf32_Ehdr *encode_ehdr(Elf32_Ehdr *header)
{
    static Elf32_Ehdr result;

    memcpy(&result.e_ident, &header->e_ident, EI_NIDENT);
    result.e_type = encode_half(header->e_type);
    result.e_machine = encode_half(header->e_machine);
    result.e_version = encode_word(header->e_version);
    result.e_entry = encode_addr(header->e_entry);
    result.e_phoff = encode_off(header->e_phoff);
    result.e_shoff = encode_off(header->e_shoff);
    result.e_flags = encode_word(header->e_flags);
    result.e_ehsize = encode_half(header->e_ehsize);
    result.e_phentsize = encode_half(header->e_phentsize);
    result.e_phnum = encode_half(header->e_phnum);
    result.e_shentsize = encode_half(header->e_shentsize);
    result.e_shnum = encode_half(header->e_shnum);
    result.e_shstrndx = encode_half(header->e_shstrndx);

    return &result;
}

static void decode_ehdr(Elf32_Ehdr *header)
{
    header->e_type = decode_half(header->e_type);
    header->e_machine = decode_half(header->e_machine);
    header->e_version = decode_word(header->e_version);
    header->e_entry = decode_addr(header->e_entry);
    header->e_phoff = decode_off(header->e_phoff);
    header->e_shoff = decode_off(header->e_shoff);
    header->e_flags = decode_word(header->e_flags);
    header->e_ehsize = decode_half(header->e_ehsize);
    header->e_phentsize = decode_half(header->e_phentsize);
    header->e_phnum = decode_half(header->e_phnum);
    header->e_shentsize = decode_half(header->e_shentsize);
    header->e_shnum = decode_half(header->e_shnum);
    header->e_shstrndx = decode_half(header->e_shstrndx);
}

static Elf32_Shdr *encode_shdr(Elf32_Shdr *header)
{
    static Elf32_Shdr result;

    result.sh_name = encode_word(header->sh_name);
    result.sh_type = encode_word(header->sh_type);
    result.sh_flags = encode_word(header->sh_flags);
    result.sh_addr = encode_addr(header->sh_addr);
    result.sh_offset = encode_off(header->sh_offset);
    result.sh_size = encode_word(header->sh_size);
    result.sh_link = encode_word(header->sh_link);
    result.sh_info = encode_word(header->sh_info);
    result.sh_addralign = encode_word(header->sh_addralign);
    result.sh_entsize = encode_word(header->sh_entsize);


    return &result;
}

static void decode_shdr(Elf32_Shdr *header)
{
    header->sh_name = decode_word(header->sh_name);
    header->sh_type = decode_word(header->sh_type);
    header->sh_flags = decode_word(header->sh_flags);
    header->sh_addr = decode_addr(header->sh_addr);
    header->sh_offset = decode_off(header->sh_offset);
    header->sh_size = decode_word(header->sh_size);
    header->sh_link = decode_word(header->sh_link);
    header->sh_info = decode_word(header->sh_info);
    header->sh_addralign = decode_word(header->sh_addralign);
    header->sh_entsize = decode_word(header->sh_entsize);
}

static Elf32_Phdr *encode_phdr(Elf32_Phdr *header)
{
    static Elf32_Phdr result;

    result.p_type = encode_word(header->p_type);
    result.p_offset = encode_off(header->p_offset);
    result.p_vaddr = encode_addr(header->p_vaddr);
    result.p_paddr = encode_addr(header->p_paddr);
    result.p_filesz = encode_word(header->p_filesz);
    result.p_memsz = encode_word(header->p_memsz);
    result.p_flags = encode_word(header->p_flags);
    result.p_align = encode_word(header->p_align);

    return &result;
}

static void decode_phdr(Elf32_Phdr *header)
{
    header->p_type = decode_word(header->p_type);
    header->p_offset = decode_off(header->p_offset);
    header->p_vaddr = decode_addr(header->p_vaddr);
    header->p_paddr = decode_addr(header->p_paddr);
    header->p_filesz = decode_word(header->p_filesz);
    header->p_memsz = decode_word(header->p_memsz);
    header->p_flags = decode_word(header->p_flags);
    header->p_align = decode_word(header->p_align);
}

static Elf32_Sym *encode_sym(Elf32_Sym *sym)
{
    static Elf32_Sym result;

    result.st_name = encode_word(sym->st_name);
    result.st_value = encode_addr(sym->st_value);
    result.st_size = encode_word(sym->st_size);
    result.st_info = sym->st_info;
    result.st_other = sym->st_other;
    result.st_shndx = encode_half(sym->st_shndx);

    return &result;
}

static void decode_sym(Elf32_Sym *sym)
{
    sym->st_name = decode_word(sym->st_name);
    sym->st_value = decode_addr(sym->st_value);
    sym->st_size = decode_word(sym->st_size);
    sym->st_shndx = decode_half(sym->st_shndx);
}

static Elf32_Rela *encode_rela(Elf32_Rela *rela)
{
    static Elf32_Rela result;

    result.r_offset = encode_addr(rela->r_offset);
    result.r_info = encode_word(rela->r_info);
    result.r_addend = encode_sword(rela->r_addend);

    return &result;
}

static void decode_rela(Elf32_Rela *rela)
{
    rela->r_offset = decode_addr(rela->r_offset);
    rela->r_info = decode_word(rela->r_info);
    rela->r_addend = decode_sword(rela->r_addend);
}

static Elf32_Half encode_half(Elf32_Half half)
{
    Elf32_Half result;
    unsigned char *ptr = (char *)(&result);
    *ptr++ = (half >> 8) & 0xFF;
    *ptr++ = (half & 0xFF);
    return result;
}

static Elf32_Word encode_word(Elf32_Word word)
{
    Elf32_Word result;
    unsigned char *ptr = (char *)(&result);
    *ptr++ = (word >> 24) & 0xFF;
    *ptr++ = (word >> 16) & 0xFF;
    *ptr++ = (word >> 8) & 0xFF;
    *ptr++ = (word & 0xFF);
    return result;
}

static Elf32_Sword encode_sword(Elf32_Sword word)
{
    Elf32_Sword result;
    unsigned char *ptr = (char *)(&result);
    *ptr++ = (word >> 24) & 0xFF;
    *ptr++ = (word >> 16) & 0xFF;
    *ptr++ = (word >> 8) & 0xFF;
    *ptr++ = (word & 0xFF);
    return result;
}

static Elf32_Off encode_off(Elf32_Off off)
{
    Elf32_Off result;
    unsigned char *ptr = (char *)(&result);
    *ptr++ = (off >> 24) & 0xFF;
    *ptr++ = (off >> 16) & 0xFF;
    *ptr++ = (off >> 8) & 0xFF;
    *ptr++ = (off & 0xFF);
    return result;
}

static Elf32_Addr encode_addr(Elf32_Addr addr)
{
    Elf32_Addr result;
    unsigned char *ptr = (char *)(&result);
    *ptr++ = (addr >> 24) & 0xFF;
    *ptr++ = (addr >> 16) & 0xFF;
    *ptr++ = (addr >> 8) & 0xFF;
    *ptr++ = (addr & 0xFF);
    return result;
}

static Elf32_Half decode_half(Elf32_Half half)
{
    Elf32_Half result = 0;
    unsigned char *ptr = (char *)(&half);
    result = result << 8 | (*ptr++);
    result = result << 8 | (*ptr++);
    return result;
}

static Elf32_Word decode_word(Elf32_Word word)
{
    Elf32_Word result = 0;
    unsigned char *ptr = (char *)(&word);
    result = result << 8 | (*ptr++);
    result = result << 8 | (*ptr++);
    result = result << 8 | (*ptr++);
    result = result << 8 | (*ptr++);
    return result;
}

static Elf32_Sword decode_sword(Elf32_Sword word)
{
    Elf32_Word result = 0;
    unsigned char *ptr = (char *)(&word);
    result = result << 8 | (*ptr++);
    result = result << 8 | (*ptr++);
    result = result << 8 | (*ptr++);
    result = result << 8 | (*ptr++);
    return result;
}

static Elf32_Off decode_off(Elf32_Off off)
{
    Elf32_Off result = 0;
    unsigned char *ptr = (char *)(&off);
    result = result << 8 | (*ptr++);
    result = result << 8 | (*ptr++);
    result = result << 8 | (*ptr++);
    result = result << 8 | (*ptr++);
    return result;
}

static Elf32_Addr decode_addr(Elf32_Addr addr)
{
    Elf32_Addr result = 0;
    unsigned char *ptr = (char *)(&addr);
    result = result << 8 | (*ptr++);
    result = result << 8 | (*ptr++);
    result = result << 8 | (*ptr++);
    result = result << 8 | (*ptr++);
    return result;
}
