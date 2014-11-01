#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <assert.h>
/*Este cabeçalho traz a definição da macro assert() que implementa uma asserção, utilizada para verificar suposições feitas pelo programa. Sempre que a expressão passada como argumento é falsa (igual a zero) então a macro escreve uma mensagem na saída padrão de erro e termina o programa chamando abort()1 .
Através da macro é possível diagnosticar problemas através da informação impressa pela macro1 que contém o nome do arquivo fonte, a linha do arquivo contendo a chamada para a macro, o nome da função que contém a chamada e o texto da expressão que foi avaliada.*/

#include "vmm.h"

/* O arranjo __frames representa a memoria fisica do computador.
 * A variavel __pagetable eh o numero do quadro na memoria fisica que contem a tabela de paginas corrente.
 * O valor desta variavel deve ser configurado pelo sistema
 * operacional chamando dccvmm_set_page_table().
 *
 * Q: Qual a memoria fisica disponivel no computador?  Qual a quantidade de
 * bits necessaria nos enderecos fisicos? */

//#define NUMFRAMES 0x1000
//static struct frame __frames[NUMFRAMES]; /* a memoria fisica possui 4.096 frames*/
//static uint32_t __pagetable; /*__pagetable eh o numero do quadro na memoria fisica que contem a tabela de paginas corrente. */

extern uint32_t os_pagefault(uint32_t address, uint32_t permissao, uint32_t pte);

/* A funcao dccvmm_set_page_table informa ao controlador de memoria em qual
 * frame esta a tabela de paginas corrente. */
void dccvmm_set_page_table(uint32_t framenum) {
    printf("vmm using pagetable in frame %x\n", framenum);
    __pagetable = framenum;
}

/* pte = page table entry */
static uint32_t dccvmm_get_pte(uint32_t frame, uint8_t ptenum, uint32_t perms, uint32_t address) {
    /* Seleciona dentro da memoria principal (__frames) o frame desejado. 
    Dentro do frame desejado, seleciona o numero da pagina que está dentro do frame */
    //printf("(RETIRAR ESTE PRINT) Entrando na função \"dccvmm_get_pte\"\n");
	//printf("(RETIRAR ESTE PRINT) frame: 0x%X\n", frame);
	//printf("(RETIRAR ESTE PRINT) ptenum: 0x%X\n", ptenum);
	//printf("(RETIRAR ESTE PRINT) perms: 0x%X\n", perms);
	//printf("(RETIRAR ESTE PRINT) address: 0x%X\n", address);
	//printf("(RETIRAR ESTE PRINT) pte = __frames[%i].words[%X] \n", frame, ptenum );
	
    uint32_t pte = __frames[frame].words[ptenum];
   	//printf("(RETIRAR ESTE PRINT) pte: 0x%X\n", pte);
    
    /*Eu acho que aqui, ele comprara a página calculada anteriormente com o vetor de permissoes para ver se esta pagina pode ser recuperada*/
    if ((pte & perms) != perms) {
        /* Q: O que acontece quando os_pagefault retorna o valor
         * VM_ABORT? Cite um exemplo onde os_pagefault retorna
         * VM_ABORT. */
        uint32_t r = os_pagefault(address, perms, pte);
        if (r == VM_ABORT) return VM_ABORT;
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

/* dccvmm_read retorna a palavra de 32-bits apontada pelo endereco virtual
 * address na tabela de paginas atual. 
 * OU SEJA, dccvmm_read devolve a entrada da tabela de paginas apontada pelo endereço virtual address
 * OU SEJA, dccvmm_read vai na tabela de pagina, no endereco apontado por address e devolve o frame daquela pagina (16 bits)
 
 */
uint32_t dccvmm_read(uint32_t address) {

	//printf("\n\n(RETIRAR ESTE PRINT) Entrando na função \"dccvmm_read\"\n");
	//printf("(RETIRAR ESTE PRINT) address: %X\n", address);
    uint32_t perms = PTE_RW | PTE_INMEM | PTE_VALID; 
    /* A operação acima eh uma combinação de:
     * PTE_RW    0x00800000  o quadro apontado pelo pte tem permissao de leitura e escrita
     * PTE_VALID 0x00100000  o endereco virtual foi alocado pelo processo
     * PTE_INMEM 0x00400000  o quadro apontado pelo pte esta na memoria
     * resultado 0x00D00000 ou seja, a permissao eh de leitura e escrita + é do processo + está na memória
     */
    
    /*PTE1OFF(address) = Separa os bitos 23 a 16 de address e os coloca na posição 7 a 0
    dccvmm_get_pte(uint32_t frame, uint8_t ptenum, uint32_t perms, uint32_t address)
     */
     
    //printf("(RETIRAR ESTE PRINT) __pagetable: 0x%X\n", __pagetable);
    //printf("(RETIRAR ESTE PRINT) PTE1OFF(address): %X\n", PTE1OFF(address));
    //printf("(RETIRAR ESTE PRINT) perms: 0x%X\n", perms);
    uint32_t pte1 = dccvmm_get_pte(__pagetable, PTE1OFF(address), perms, address);
    if (pte1 == VM_ABORT) return 0;
    uint32_t pte1frame = PTEFRAME(pte1); /*Separa os 12 bits menos significativos de pte1*/ 
    
    /*PTE2OFF(address) = Separa os bitos 15 a 8 de address e os coloca na posição 7 a 0
    dccvmm_get_pte(uint32_t frame, uint8_t ptenum, uint32_t perms, uint32_t address)
     */
    uint32_t pte2 = dccvmm_get_pte(pte1frame, PTE2OFF(address), perms, address);
    if (pte2 == VM_ABORT) return 0;
    uint32_t pte2frame = PTEFRAME(pte2); /*Separa os 12 bits menos significativos de pte2*/ 
    uint32_t data = __frames[pte2frame].words[PAGEOFFSET(address)]; /*Separa os bitos 7 a 0 de address*/
    printf("vmm %x phy %x read %x\n", address,
            (pte2frame << 8) + PAGEOFFSET(address), data); /*Separa os bitos 7 a 0 de address*/
    return data;
}

/* dccvmm_phy_read retorna a palavra de 32-bits apontada pelo endereco
 * fisico phyaddr, transpassando o sistema de memoria */
uint32_t dccvmm_phy_read(uint32_t phyaddr) {
    assert((phyaddr >> 8) < NUMFRAMES);
    uint32_t data = __frames[phyaddr >> 8].words[PAGEOFFSET(phyaddr)];
    printf("vmm phy %x read %x\n", phyaddr, data);
    return data;
}

/* dccvmm_write escreve data na palavra de 32-bits apontada pelo endereco
 * virtual address na tabela de paginas atual. */
void dccvmm_write(uint32_t address, uint32_t data) {
	//printf("\n\n(RETIRAR ESTE PRINT) Entrando na função \"dccvmm_write\"\n");
    uint32_t perms = PTE_RW | PTE_INMEM | PTE_VALID;
    uint32_t pte1 = dccvmm_get_pte(__pagetable, PTE1OFF(address), perms, address);
    if (pte1 == VM_ABORT) return;
    uint32_t pte1frame = PTEFRAME(pte1);
    uint32_t pte2 = dccvmm_get_pte(pte1frame, PTE2OFF(address), perms, address);
    if (pte2 == VM_ABORT) return;
    uint32_t pte2frame = PTEFRAME(pte2);
    __frames[pte2frame].words[PAGEOFFSET(address)] = data;
    printf("vmm %x phy %x write %x\n", address, (pte2frame << 8) + PAGEOFFSET(address), data);
}

/* dccvmm_phy_write escreve data na palavra de 32-bits apontada pelo endereco
 * fisico phyaddr, transpassando o sistema de memoria. */

void dccvmm_phy_write(uint32_t phyaddr, uint32_t data) {
    assert((phyaddr >> 8) < NUMFRAMES);
    __frames[phyaddr >> 8].words[PAGEOFFSET(phyaddr)] = data;
    printf("vmm phy %x write %x\n", phyaddr, data);
}

/* dccvmm_zero zera um quadro na memoria fisica, transpassando o sistema de
 * memoria virtual. */
void dccvmm_zero(uint32_t framenum) {
    memset(&(__frames[framenum]), 0, sizeof (__frames[0]));
}

/*****************************************************************************
 * Interface com o disco
 ****************************************************************************/
struct sector {
    uint32_t words[0x100]; /*Um setor do disco possui a mesma dimensão de um frame = 512 posições de 32 bits cada = 2KB*/
};

static struct sector *__disk; /*apontador de uma struct tipo sector chamada __disk*/

void dccvmm_init(void) {
    __disk = malloc(0x00100000 * sizeof (*__disk));
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
 * memoria virtual suporta? 
 * R: O disco possui 2^17 = 131.072 = 128K setores, onde cada setor possui o mesmo tamanho de um frame = 2KB. Portanto, temos um disco de tamanho 256MB.
 */

/* As funcoes dccvmm_dump_frame e load_frame gravam e carregam um quadro de 
 * memoria fisica no disco rigido.  Para simplificar definimos que cada setor
 * do disco tem o mesmo tamanho de um quadro de memoria fisica. */
void dccvmm_dump_frame(uint32_t framenum, uint32_t sector) {
    /*memcpy(destino, origem, tamanho a ser copiado)*/
    memcpy(&(__disk[sector]), &(__frames[framenum]), sizeof (__disk[0]));
}

void dccvmm_load_frame(uint32_t sector, uint32_t framenum) {
    /*memcpy(destino, origem, tamanho a ser copiado)*/
    /*viguirilo: possivel erro detectado: troquei o código comentado abaixo pelo que está logo abaixo dele. deste modo, a função laod carrega
     uma informação do disco para memória, e não ao contrário como estava antes*/
    /*memcpy(&(__disk[sector]), &(__frames[framenum]), sizeof (__disk[0]));*/
    memcpy(&(__frames[framenum]), &(__disk[sector]), sizeof (__disk[0]));
}
