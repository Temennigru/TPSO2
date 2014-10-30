#ifndef __VMM_HEADER__
#define __VMM_HEADER__

#include <inttypes.h>

/*
inttypes.h - fixed size integer types

The purpose of <inttypes.h> is to provide a set of integer types whose definitions are consistent across machines and independent of operating systems and other implementation idiosyncrasies. It defines, via typedef, integer types of various sizes. Implementations are free to typedef them as ISO C standard integer types or extensions that they support. Consistent use of this header will greatly increase the portability of applications across platforms.

The <inttypes.h> header shall include the <stdint.h> header.

stdint.h - integer types

The <stdint.h> header shall declare sets of integer types having specified widths, and shall define corresponding sets of macros. It shall also define macros that specify limits of integer types corresponding to types defined in other standard headers.

The typedef name uint N _t designates an unsigned integer type with width N. Thus, uint32_t denotes an unsigned integer type with a width of exactly 32 bits.

 */

/* Estrutura dos enderecos virtuais (armazenados num uint32_t):
 * 23             7        0
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
 * R: 4.294.967.296 (4 gigas) posições de memória de 32 bits (4 bytes).
 *
 * Q: As macros abaixo extraem informacoes de um endereco virtual.  Explique
 * qual informacao eh extraida por cada macro. 
 * R: O nosso esquema oferece uma sistema de paginação multinível onde há uma paginação na memória e outra paginação entre as páginas que estarão presentes na memória. A macro PTE1OFF(addr) identifica se o endereço passado coorresponde à mesma seção em que a página desejada se encontra. A macro PTE2OFF(addr) identifica qual página daquela seção é desejada. E a macro PAGEOFFSET(addr) identifica o offset dentro da página desejada.
 */
#define PTE1OFF(addr) ((addr & 0x00ff0000) >> 16) /*Separa os bitos 23 a 16 de addr e os coloca na posição 7 a 0*/
#define PTE2OFF(addr) ((addr & 0x0000ff00) >> 8)  /*Separa os bitos 15 a 8 de addr e os coloca na posição 7 a 0*/
#define PAGEOFFSET(addr) (addr & 0x000000ff)      /*Separa os bitos 7 a 0 de addr*/

/* O struct frame abaixo define um quadro de memoria fisica. Quadros de
 * memoria fisica sao a menor unidade controlada pelo controlador de memoria.
 *
 * Q: Qual o tamanho em bytes de cada quadro de memoria fisica?
 * R: Um quadro é formado por um array de 256 elementos do tipo uint32_t que possui 32 bits (4 bytes). Portanto, cada frame possui 256 elementos de 4 bytes = 1024 bytes ou 1KB
 *
 * Q: Quantas palavras existem em cada quadro de memoria fisica? 
 * R: Cada quadro possui 256 elementos de 4 bytes. Portanto, existem 256 palavras dentro de cada quadro de memória física.
 */

struct frame {
    uint32_t words[0x100]; /* uint32_t é um tipo de dados de 32 bits sem sinal 
                            * Um frame possui 256 palavras de 4 bytes
                            */
};

#define NUMFRAMES 0x1000
struct frame __frames[NUMFRAMES]; /* a memoria fisica possui 4.096 frames*/
uint32_t __pagetable; /*__pagetable eh o numero do quadro na memoria fisica que contem a tabela de paginas corrente. */

/* Estado de uma entrada na tabela de paginas (page table entry, pte):
 *   PTE_VALID: o endereco virtual foi alocado pelo processo
 *   PTE_DIRTY: o quadro apontado pelo pte foi modificado
 *   PTE_INMEM: o quadro apontado pelo pte esta na memoria
 *   PTE_RW: o quadro apontado pelo pte tem permissao de leitura e escrita */
#define PTE_VALID 0x00100000
#define PTE_DIRTY 0x00200000
#define PTE_INMEM 0x00400000
#define PTE_RW    0x00800000

#define PTEFRAME(pte) (pte & 0x00000fff) /*Separa os 12 bits menos significativos de pte*/ 
#define PTEUSER(pte) (pte & 0x7f000000) /*Separa os bits 30 a 24 de pte*/
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

#endif
