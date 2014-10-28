#ifndef __VMM_HEADER__
#define __VMM_HEADER__

#include <inttypes.h>

/* Estrutura dos enderecos virtuais (armazenados num uint32_t):
 * 24             8        0
 * +--------------+--------+
 * | page number  | offset |
 * +--------------+--------+ */

/* Em nossa arquitetura simulada, enderecos virtuais enumeram palavras de 32
 * bits.  Por exemplo, o endereco 0x0 recupera a primeira palavra de 32 bits
 * da memoria (bytes 0, 1, 2 e 3); o endereco 0x3 recupera a quarta palavra de
 * 32 bits na memoria (bytes 12, 13, 14 e 15) e o endereco 0x4 recupera a
 * quinta palavra de 32 bits (bytes 16, 17, 18 e 19).
 *
 * Q: Qual o maximo de memoria que um processo pode enderecar?
 *
 * Q: As macros abaixo extraem informacoes de um endereco virtual.  Explique
 * qual informacao eh extraida por cada macro. */
#define PTE1OFF(addr) ((addr & 0x00ff0000) >> 16)
#define PTE2OFF(addr) ((addr & 0x0000ff00) >> 8)
#define PAGEOFFSET(addr) (addr & 0x000000ff)

/* O struct frame abaixo define um quadro de memoria fisica. Quadros de
 * memoria fisica sao a menor unidade controlada pelo controlador de memoria.
 *
 * Q: Qual o tamanho em bytes de cada quadro de memoria fisica?
 *
 * Q: Quantas palavras existem em cada quadro de memoria fisica? */
struct frame {
    uint32_t words[0x100];
};

/* Estado de uma entrada na tabela de paginas (page table entry, pte):
 *   PTE_VALID: o endereco virtual foi alocado pelo processo
 *   PTE_DIRTY: o quadro apontado pelo pte foi modificado
 *   PTE_INMEM: o quadro apontado pelo pte esta na memoria
 *   PTE_RW: o quadro apontado pelo pte tem permissao de leitura e escrita */
#define PTE_VALID 0x00100000
#define PTE_DIRTY 0x00200000
#define PTE_INMEM 0x00400000
#define PTE_RW    0x00800000

#define PTEFRAME(pte) (pte & 0x00000fff)
#define PTEUSER(pte) (pte & 0x7f000000)
/* Os bits em PTEUSER sao de uso livre pelo sistema operacional. */

/* O valor VM_ABORT deve ser usado retornado pela funcao os_pagefault quando o
 * sistema operacional precisar cancelar o acesso a memoria que causou a falha
 * de pagina. */
#define VM_ABORT 0x80000000

void dccvmm_init(void);

/* A funcao dccvmm_set_page_table informa ao controlador de memoria em qual
 * frame esta a tabela de paginas corrente. */
void dccvmm_set_page_table(uint32_t framenum);

/* dccvmm_read retorna a palavra de 32-bits apontada pelo endereco virtual
 * address na tabela de paginas atual. */
uint32_t dccvmm_read(uint32_t address);

/* dccvmm_phy_read retorna a palavra de 32-bits apontada pelo endereco
 * fisico phyaddr, transpassando o sistema de memoria */
uint32_t dccvmm_phy_read(uint32_t phyaddr);

/* dccvmm_write escreve data na palavra de 32-bits apontada pelo endereco
 * virtual address na tabela de paginas atual. */
void dccvmm_write(uint32_t address, uint32_t data);

/* dccvmm_phy_write escreve data na palavra de 32-bits apontada pelo endereco
 * fisico phyaddr, transpassando o sistema de memoria. */
void dccvmm_phy_write(uint32_t phyaddr, uint32_t data);

/* dccvmm_zero zera um quadro na memoria fisica, transpassando o sistema de
 * memoria virtual. */
void dccvmm_zero(uint32_t framenum);

/* As funcoes dccvmm_dump_frame e load_frame gravam e carregam um quadro de
 * memoria fisica no disco rigido.  Para simplificar definimos que cada setor
 * do disco tem o mesmo tamanho de um quadro de memoria fisica. */
void dccvmm_dump_frame(uint32_t framenum, uint32_t sector);
void dccvmm_load_frame(uint32_t sector, uint32_t framenum);

#endif /* defined(__TPSO2__vmm__) */