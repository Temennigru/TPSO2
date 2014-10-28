#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <assert.h>

#include "vmm.h"

/* O arranjo __frames representa a memoria fisica do computador.  A variavel
 * __pagetable eh o numero do quadro na memoria fisica que contem a tabela de
 * paginas corrente.  O valor desta variavel deve ser configurado pelo sistema
 * operacional chamando dccvmm_set_page_table().
 *
 * Q: Qual a memoria fisica disponivel no computador?  Qual a quantidade de
 * bits necessaria nos enderecos fisicos? */

#define NUMFRAMES 0x1000
static struct frame __frames[NUMFRAMES];
static uint32_t __pagetable;

extern uint32_t os_pagefault(uint32_t address, uint32_t perms, uint32_t pte);

void dccvmm_set_page_table(uint32_t framenum) {
    printf("vmm using pagetable in frame %x\n", framenum);
    __pagetable = framenum;
}

static uint32_t dccvmm_get_pte(uint32_t frame, uint8_t ptenum,
                               uint32_t perms, uint32_t address)
{
    uint32_t pte = __frames[frame].words[ptenum];
    if((pte & perms) != perms) {
        /* Q: O que acontece quando os_pagefault retorna o valor
         * VM_ABORT? Cite um exemplo onde os_pagefault retorna
         * VM_ABORT. */
        uint32_t r = os_pagefault(address, perms, pte);
        if(r == VM_ABORT) return VM_ABORT;
    }
    pte = __frames[frame].words[ptenum];
    return pte;
    /* Q: Por que o valor pte (page table entry) nunca sera confundido com
     * o valor de VM_ABORT que pode ser retornado no if acima? */
}

/* Q: Descreva o funcionamento o controlador de memoria analisando o codigo
 * das funcoes dccvmm_read, dccvmm_write, e dccvmm_get_pte.  Explique como um
 * endereco virtual eh convertido num endereco fisico.  Faca um diagrama da
 * tabela de paginas.
 *
 * Q: Como o sistema operacional pode ignorar o controle de permissoes do
 * controlador de memoria? */

uint32_t dccvmm_read(uint32_t address)
{
    uint32_t perms = PTE_RW | PTE_INMEM | PTE_VALID;
    uint32_t pte1 = dccvmm_get_pte(__pagetable,
                                   PTE1OFF(address), perms, address);
    if(pte1 == VM_ABORT) return 0;
    uint32_t pte1frame = PTEFRAME(pte1);
    uint32_t pte2 = dccvmm_get_pte(pte1frame,
                                   PTE2OFF(address), perms, address);
    if(pte2 == VM_ABORT) return 0;
    uint32_t pte2frame = PTEFRAME(pte2);
    uint32_t data = __frames[pte2frame].words[PAGEOFFSET(address)];
    printf("vmm %x phy %x read %x\n", address,
           (pte2frame << 8) + PAGEOFFSET(address), data);
    return data;
}

uint32_t dccvmm_phy_read(uint32_t phyaddr)
{
    assert((phyaddr >> 8) < NUMFRAMES);
    uint32_t data = __frames[phyaddr >> 8].words[PAGEOFFSET(phyaddr)];
    printf("vmm phy %x read %x\n", phyaddr, data);
    return data;
}

void dccvmm_write(uint32_t address, uint32_t data)
{
    uint32_t perms = PTE_RW | PTE_INMEM | PTE_VALID;
    uint32_t pte1 = dccvmm_get_pte(__pagetable,
                                   PTE1OFF(address), perms, address);
    if(pte1 == VM_ABORT) return;
    uint32_t pte1frame = PTEFRAME(pte1);
    uint32_t pte2 = dccvmm_get_pte(pte1frame,
                                   PTE2OFF(address), perms, address);
    if(pte2 == VM_ABORT) return;
    uint32_t pte2frame = PTEFRAME(pte2);
    __frames[pte2frame].words[PAGEOFFSET(address)] = data;
    printf("vmm %x phy %x write %x\n", address,
           (pte2frame << 8) + PAGEOFFSET(address), data);
}


void dccvmm_phy_write(uint32_t phyaddr, uint32_t data)
{
    assert((phyaddr >> 8) < NUMFRAMES);
    __frames[phyaddr >> 8].words[PAGEOFFSET(phyaddr)] = data;
    printf("vmm phy %x write %x\n", phyaddr, data);
}


void dccvmm_zero(uint32_t framenum)
{
    memset(&(__frames[framenum]), 0, sizeof(__frames[0]));
}

/*****************************************************************************
 * Interface com o disco
 ****************************************************************************/
struct sector {
    uint32_t words[0x100];
};

static struct sector *__disk;

void dccvmm_init(void) {
    __disk = malloc(0x00100000 * sizeof(*__disk));
    assert(__disk);
}

/* O arranjo __disk acima representa o disco do computador, que sera utilizado
 * para implementacao do sistema de memoria virtual.  O disco so pode ser
 * escrito um setor por vez.  Para facilitar, definimos que o setor do disco
 * tem o mesmo tamanho de um quadro de memoria.  As funcoes seguintes servem
 * para copiar e carregar quadros do disco.  Seu sistema operacional deve
 * utilizar estas funcoes para fazer do disco uma extensao da memoria virtual.
 *
 * Q: Qual o tamanho do disco?  Qual o minimo de processos que o sistema de
 * memoria virtual suporta? */

void dccvmm_dump_frame(uint32_t framenum, uint32_t sector)
{
    memcpy(&(__disk[sector]), &(__frames[framenum]), sizeof(__disk[0]));
}

void dccvmm_load_frame(uint32_t sector, uint32_t framenum)
{
    memcpy(&(__disk[sector]), &(__frames[framenum]), sizeof(__disk[0]));
}