#ifndef TPSO2_tp2_h
#define TPSO2_tp2_h

// Cabeçalho das funções:
void os_init(void);
uint32_t os_pagefault(uint32_t address, uint32_t perms, uint32_t pte);
void os_alloc(uint32_t virtaddr);
void os_free(uint32_t virtaddr);
void os_swap(uint32_t pid);
uint32_t procurar_frame_livre_dados(void);
uint32_t procurar_frame_sistema(void);
uint32_t procurar_frame_tabela_2(uint32_t virtaddr);

#endif
