#include <stdlib.h>
#include <stdio.h>

#include "tp2.h"
#include "vmm.h"

#define TAMANHO_FRAME 0x100
#define INICIO_MEMORIA_SISTEMA 0x0 // endereço do primeiro frame
// O primeiro meio frame da memória (0x0 a 0x7F será destinado para registrar os frames livres.
// Cada bit deste meio frame representará um frame na memória: 0.5 frame (128) X 32 bits (word) = 4.096 bits = quantidade de frames presentes na memória.
#define INICIO_FRAMES_LIVRES 0x0 // endereço do primeiro frame
#define FIM_FRAMES_LIVRES 0x7F // 0x7F = 0d127 endereço de memoria
#define INICIO_TABELA_SISTEMA 0x200 // 0x200 = 0d512 endereço de memoria
#define FIM_TABELA_SISTEMA 0xFFF // 0xFFF = 0d4095 endereço de memoria
#define FIM_MEMORIA_SISTEMA 0xFFF // 0xFFF = 0d4095 endereço de memoria
#define INICIO_MEMORIA_PROCESSOS 0x1000 // 0x1000 = 0d4096 endereço de memoria
#define FIM_MEMORIA_PROCESSOS 0xFFFFF // 0xFFFFF = 1048575 endereço de memoria


uint32_t procurar_frame_livre_dados(void);
uint32_t procurar_frame_sistema(void);
uint32_t procurar_frame_tabela_2(uint32_t virtaddr);
uint32_t dump_setor_livre (uint32_t);

static uint32_t id_processos = 1;


void os_init(void) {	
	uint32_t i;
	// Inicializa os a memória de sistema:
	// tentar usar a funcao dccvmm_zero
	for(i=0; i<16; i++)
	{
		dccvmm_zero(i);
	}
	
	/* A estrutura que identifica os frames livres será da seguinte maneira:
	 *	A memória possui 4096 frames;
	 *	Pegaremos os 4096 primeiros bits da memória de sistema e mapearemos cada bit coorrespondente a um frame da memória;
	 *	bit == 0: frame livre; bit == 1: frame ocupado;
	 *	Portanto, usaremos meio frame para mapear os frames livres (128 entradas de 32 bits = 4096 bits);
	*/
	// Inicializa a estrutura que identifica os frames livres, informando que os 16 primeiros frames estão ocupados (pelo sistema):
	__frames[0].words[0] |= 0xFFFF;
	// Inicializando a variável __pagetable:
	__pagetable = 0x0;
	// Inicializando o contador do id de processos: Em nosso sistema operacional, não é aceito um processo com ID zero
    id_processos = 1;


    // Inicializar o disco:
    dccvmm_init();

    // Usa frame 1 para inicializar o disco
    uint32_t j;
    for (j = 0; j < 0x80; j++) {
        __frames[1].words[j] = 0xFFFF; // Primeiros 128 frames indicam o uso do disco
    }

    dccvmm_dump_frame(0x1, 0x0); // Dump do frame 2 para o setor 0

    dccvmm_zero(1); // Reset no frame 1

    for (j = 0x1; j < 0x100; j++) {
        dccvmm_dump_frame(0x1, j); //
    }

}

uint32_t os_pagefault(uint32_t address, uint32_t perms, uint32_t pte){
	// Verifica se há uma entrada válida na tabela de páginas:
	// acho que tenho que pensar melhor nesse caso para quando chamar o os_alloc pois neste caso
	if(pte < 0x10)
	{
		printf("Erro de segmentação: Não existe entrada válida na tabela de páginas para o endereço virtual 0x%X\n", address);
		printf("(RETIRAR ESTE PRINT) pte: 0x%X\n", pte); 
		return VM_ABORT;
	}
	// Verifica se as permissões estão compatíveis:
	// acho que tenho que pensar melhor nesse caso
	if(perms != (pte & 0xF00000))
	{
		printf("Erro de segmentação: Erro de permissões\n");
		//return VM_ABORT;
	}
	return EXIT_SUCCESS;
}

void os_alloc(uint32_t virtaddr) {
	printf("\n\n(RETIRAR ESTE PRINT) Função \"os_alloc\"\n");
	// Verifica se endereço virtual é múltiplo do tamanho do frame:
	if(virtaddr%TAMANHO_FRAME)
	{
		printf("Erro de segmentação: Não foi possível alocar o endereço 0x%X pois ele não é múltiplo do tamanho do frame 0x%X\n", virtaddr, TAMANHO_FRAME);
		return;
	}
	uint32_t pte = 0x0;
	uint32_t mascara;
	uint32_t linha_livre_ts = 0x0; // linha livre tabela de sistema para alocar a tabela 1
	uint32_t frame_tabela1 = 0x0; // frame livre tabela de página 1
	uint32_t frame_tabela2 = 0x0; // frame livre tabela de página 1
	uint32_t frame_livre_dado = 0x0; // frame livre onde vai o dado propriamente dito
	printf("(RETIRAR ESTE PRINT) Endereço virtual:  0x%X\n", virtaddr);
	
	// Procura por uma linha livre na memória de sistema para alocar os dados da tabela 1:
	linha_livre_ts = procurar_frame_sistema();
	// Verifica se existe uma entrada na tabela de sistema para o processo atual. Se não existir, verifica se há espaço disponível para alocar as informações do processo atual.
	if(linha_livre_ts)
	{
		// Não existiam informações na tabela de sistema para o processo atual:
		if(__pagetable == 0x0)
		{
			// Procura por um frame livre na memoria de dados para alocar a tabela de página 1:
			frame_tabela1 = procurar_frame_livre_dados();
		}
		// Existia informações na tabela de sistema para o processo atual:
		else
		{
			frame_tabela1 = PTEFRAME(__frames[linha_livre_ts/256].words[linha_livre_ts%256]);
		}
		if(frame_tabela1)
		{
			printf("(RETIRAR ESTE PRINT) A TABELA 1 está no FRAME 0x%X da TABELA DE DADOS\n", frame_tabela1);
			// Se for uma tabela de ágina 1 que estiver entrando agora no mapa de memória, seu frame deve ser atribuido à variável global __pagetable:
			if(__pagetable == 0x0)
			{
				dccvmm_set_page_table(frame_tabela1);
			}
			// configura as permissões para ler e escrever, diz que está em memória e que é valido:
			uint32_t perms = PTE_RW | PTE_INMEM | PTE_VALID;
			// Compõe a entrada na tabela de página do sistema operacional: id do processo (8bits) + permissões (4 bits) + 8 bits mais significativos do endereço virtual + frame livre (12 bits) = 8 + 4 + 8 + 12 = 32 bits.
			pte = pte | (id_processos << 24) | perms | (virtaddr & 0x00FF0000) >> 4 | __pagetable;
			// Preenche tabela de sistema com os dados da tabela de página 1:
			printf("(RETIRAR ESTE PRINT) Inserindo os DADOS da TABELA 1 na TABELA DE SISTEMA...\n");
			__frames[linha_livre_ts/256].words[linha_livre_ts%256] = pte;			
			printf("(RETIRAR ESTE PRINT) __frames[0x%X].words[0x%X] = 0x%X;\n", linha_livre_ts/256, linha_livre_ts%256, __frames[linha_livre_ts/256].words[linha_livre_ts%256]);
			// Depois que a tabela 1 é encontrada ou criada, ele procura pela tabela 2 dentro da tabela 1:
			if(PTEFRAME(__frames[frame_tabela1].words[PTE1OFF(virtaddr)]) == 0x0)
			{
				// A tabela 2 não foi encontrada dentro da tabela 1. Procura por um frame livre na memoria de dados para alocar a tabela de página 2:
				frame_tabela2 = procurar_frame_livre_dados();
			}
			else
			{
				// A tabela 2 foi encontrada dentro da tabela 1:
				frame_tabela2 = PTEFRAME(__frames[frame_tabela1].words[PTE1OFF(virtaddr)]);
			}
			if(frame_tabela2)
			{
				printf("(RETIRAR ESTE PRINT) A TABELA 2 está no FRAME 0x%X da TABELA DE DADOS\n", frame_tabela2);
				pte = 0x0;
				pte = pte | (id_processos << 24) | perms | (virtaddr & 0x0000FF00) << 4 | frame_tabela2;
				printf("(RETIRAR ESTE PRINT) Inserindo os DADOS da TABELA 2 na TABELA 1...\n");
				__frames[frame_tabela1].words[(virtaddr & 0x00FF0000) >> 16] = pte;
				printf("(RETIRAR ESTE PRINT) __frames[0x%X].words[0x%X]: PTE 0x%X\n",frame_tabela1, (virtaddr & 0x00FF0000) >> 16, __frames[frame_tabela1].words[(virtaddr & 0x00FF0000) >> 16]);
				// Depois que a tabela 2 é encontrada ou criada, ele procura pelo dado dentro da tabela 2:
				if(PTEFRAME(__frames[frame_tabela2].words[PTE2OFF(virtaddr)]) == 0x0)
				{
					// O dado não foi encontrado dentro da tabela 2. Procura por um frame livre na memoria de dados para alocar o dado:
					frame_livre_dado = procurar_frame_livre_dados();
				}
				else
				{
					// O dado não foi encontrado dentro da tabela 2:
					frame_livre_dado = PTEFRAME(__frames[frame_tabela2].words[PTE2OFF(virtaddr)]);
				}
				if(frame_livre_dado)
				{
					printf("(RETIRAR ESTE PRINT) O DADO foi colocada no FRAME 0x%X da TABELA DE DADOS\n", frame_livre_dado);
					pte = 0x0;
					pte = pte | (id_processos << 24) | perms | (virtaddr & 0x0000FF00) << 4 | frame_livre_dado;
					printf("(RETIRAR ESTE PRINT) Inserindo os DADOS DO FRAME ALOCADO PARA DADOS na TABELA 2...\n");
					__frames[frame_tabela2].words[(virtaddr & 0x0000FF00) >> 8] = pte;
					printf("(RETIRAR ESTE PRINT) __frames[0x%X].words[0x%X]: PTE 0x%X\n",frame_tabela2, (virtaddr & 0x0000FF00) >> 8, __frames[frame_tabela2].words[(virtaddr & 0x0000FF00) >> 8]);
				}
				else
				{
					printf("(RETIRAR ESTE PRINT) Não houve espaço de MEMÓRIA DE DADOS para alocar o DADO propriamente dito\n");
					// Podemos colocar aqui uma condição para o caso de não achar um frame livre e implementar a parte 5
                    
				}
			}
			else
			{
				printf("(RETIRAR ESTE PRINT) Não houve espaço de MEMÓRIA DE DADOS para alocar a TABELA 2\n");
				// Podemos colocar aqui uma condição para o caso de não achar um frame livre e implementar a parte 5
			}
		}
		else
		{
			printf("(RETIRAR ESTE PRINT) Não houve espaço de MEMÓRIA DE DADOS para alocar a TABELA 1\n");
			// Podemos colocar aqui uma condição para o caso de não achar um frame livre e implementar a parte 5
		}
	}
	else
	{
		printf("(RETIRAR ESTE PRINT) Não houve espaço de MEMÓRIA DE SISTEMA para alocar a TABELA 1\n");
		// Podemos colocar aqui uma condição para o caso de não achar um frame livre e implementar a parte 5
	}
}

// Assuminado que o free irá liberar a memória do processo corrente, independende do ID:
void os_free(uint32_t virtaddr) {
	printf("\n\n(RETIRAR ESTE PRINT) Entrando na função \"os_free\"\n");
	uint32_t i;
	uint32_t j;
	if(virtaddr%TAMANHO_FRAME)
	{
		printf("Erro de segmentação: Não foi possível liberar o endereço 0x%X pois ele não é múltiplo do tamanho do frame 0x%X\n", virtaddr, TAMANHO_FRAME);
		return;
	}
	printf("(RETIRAR ESTE PRINT) virtaddr: 0x%X\n", virtaddr);
	// Verifica se a tabela de páginas 1 é um frame válido:
	if(__pagetable < 0x10)
	{
		printf("(RETIRAR ESTE PRINT) O frame que indica a TABELA 1 é inválido. Execução abortada.\n");
		return;
	}
	printf("(RETIRAR ESTE PRINT) A TABELA 1 (Processo atual) está no FRAME: 0x%X\n", __pagetable);
	// Percorre a tabela de páginas 1 procurando pelo frame da tabela 2:
	// A tabela de páginas 1 do processo atual está guardada na variável global "__pagetable".
	uint32_t frame_tabela2 = PTEFRAME(__frames[__pagetable].words[PTE1OFF(virtaddr)]);
	printf("(RETIRAR ESTE PRINT) A TABELA 2 (Processo atual) está no FRAME: 0x%X\n", frame_tabela2);
	if(frame_tabela2 < 0x10)
	{
		printf("(RETIRAR ESTE PRINT) O frame que indica a TABELA 2 é inválido. Execução abortada.\n");
		return;
	}
	// Percorre a tabela de páginas 2 procurando pelo frame do dado:
	struct frame *tabela2 = &(__frames[frame_tabela2]);
	uint32_t frame_dado = PTEFRAME(tabela2->words[PTE2OFF(virtaddr)]);
	printf("(RETIRAR ESTE PRINT) O FRAME DO DADO (referente ao endereço virtual 0x%X do processo atual) está no FRAME: 0x%X\n", virtaddr, frame_dado);

	// Atualiza a estrutura de frames livres:
	__frames[0].words[frame_dado/32] &= ~(0x1 << (frame_dado%32)); 
	printf("(RETIRAR ESTE PRINT) Entrada na tabela de frames livres atualizada: __frames[0].words[%i] = 0x%X\n", frame_dado/32, __frames[0].words[frame_dado/32] );
	// Atualiza a entrada da tabela de páginas 2 referente ao dado que foi liberado:
	tabela2->words[PTE2OFF(virtaddr)] = 0x0;
	// Verifica se a tabela de páginas 2 ficou vazia:
	for(i = 0x0; i<TAMANHO_FRAME; i++)
	{
		if(tabela2->words[i])
		{
			printf("(RETIRAR ESTE PRINT) A TABELA 2 não será apagada pois não ficou vazia depois da liberação do FRAME 0x%X.\n", frame_dado);
			return;
		}
	}
	printf("(RETIRAR ESTE PRINT) A TABELA 2 será apagada pois ficou vazia depois da liberação do FRAME 0x%X.\n", frame_dado);
	// Se tabela de páginas 2 ficar vazia, libera a tabela de páginas 2 da memoria de dados:
	__frames[0].words[frame_tabela2/32] &= ~(0x1 << (frame_tabela2%32));
	printf("(RETIRAR ESTE PRINT) Entrada na tabela de frames livres atualizada: __frames[0].words[%i] = 0x%X\n", frame_tabela2/32, __frames[0].words[frame_tabela2/32] );
	// Atualiza a entrada da tabela de páginas 1 referente à tabela de páginas 2 que foi liberada:
	__frames[__pagetable].words[PTE1OFF(virtaddr)] = 0x0;
	// Verifica se a tabela de páginas 1 ficou vazia:
	for(i = 0x0; i<TAMANHO_FRAME; i++)
	{
		if(__frames[__pagetable].words[i])
		{
			printf("(RETIRAR ESTE PRINT) A TABELA 1 não será apagada pois não ficou vazia depois da liberação da TABELA 2 que estava no FRAME 0x%X.\n", frame_tabela2);
			return;
		}
	}
	printf("(RETIRAR ESTE PRINT) A TABELA 1 será apagada pois ficou vazia depois da liberação da TABELA 2 que estava no FRAME 0x%X.\n", frame_tabela2);
	// Se tabela de páginas 1 ficar vazia, libera a tabela de páginas 1 da memoria de dados:
	__frames[0].words[__pagetable/32] &= ~(0x1 << (__pagetable%32));
	printf("(RETIRAR ESTE PRINT) Entrada na tabela de frames livres atualizada: __frames[0].words[%i] = 0x%X\n", __pagetable/32, __frames[0].words[__pagetable/32]);
	// Percorre a tabela de sistema para apagar a entrada da tabela de páginas 1, liberando memória para processos futuros:
	for(i=0x0; i<16; i++)
	{
		(i==0) ? (j=INICIO_TABELA_SISTEMA) : (j=0);
		for(; j<=FIM_TABELA_SISTEMA ; j++)
		{
			if((__frames[i].words[j] & 0xFFF) == __pagetable)
			{
				printf("(RETIRAR ESTE PRINT) A entrada na tabela de sistema __frames[%i].words[%i] = 0x%X referente à TABELA 1 será apagada\n", i, j, __frames[i].words[j]);
				__frames[i].words[j] = 0x0;
			}
		}
	}
	// Redefine o apontador global da tabela de páginas 1 do processo atual de modo que não haja acesso inválido enquanto outro processo não alocar memória ou houver um swap:
	__pagetable = 0x0;
	return;
}

void os_swap(uint32_t pid){
	id_processos = pid;
	procurar_frame_sistema();
}

// Procura por uum frame livre na memória de dados:
uint32_t procurar_frame_livre_dados(void){
	uint32_t i;
	uint32_t j;
	uint32_t mascara;
	// Como a estrutura de frames livres ocupa apenas meio frame, teremos que procurar dentro do frame 0 da memória de sistema.
	// Dentro do frame 0, procuramos em uma das 128 primeiras linhas:
	printf("(RETIRAR ESTE PRINT) Procurando por um FRAME LIVRE na TABELA DE DADOS...\n");
	for(i = INICIO_FRAMES_LIVRES; i<=FIM_FRAMES_LIVRES; i++)
	{
		// Dentro do frame 0, linha i, procura o bit referente àquele frame. Os bits variam de 0 a 31:
		for(j=0x0; j<32; j++)
		{
			// Faz uma máscara relativa àquele frame em questão para ver se ele está livre ou não:
			mascara = 0x1 << j;
			if((__frames[0].words[i] & mascara) == 0)
			{
				// Se o frame estiver livre, ele deve ser dado como ocupado:
				__frames[0].words[i] |= mascara;
				// E o frame que foi encontrado é atribuido à variável de frame_livre:
				printf("(RETIRAR ESTE PRINT) O FRAME 0x%X da TABELA DE DADOS está DISPONÍVEL\n", (32*i)+j);
				return (32*i)+j;
			}
		}
	}
	return 0x0;
}

uint32_t procurar_frame_sistema(void){
	uint32_t i;
	uint32_t j;
	printf("(RETIRAR ESTE PRINT) Procurando pela linha da TABELA DE SISTEMA que contém as INFORMAÇÕES DA TABELA 1 DO PROCESSO %i...\n", id_processos);
	for(i=0x0; i<16; i++)
	{
		(i==0) ? (j=INICIO_TABELA_SISTEMA) : (j=0);
		for(; j<=FIM_TABELA_SISTEMA ; j++)
		{
			if((__frames[i].words[j] & id_processos << 24) == id_processos << 24)
			{
				printf("(RETIRAR ESTE PRINT) OS DADOS DA TABELA 1 foram encontrados na linha %i DA TABELA DE SISTEMA\n", (i*256) + j);
							// Seta variável global com o frame da tabela de pagina 1 do processo atual:
				dccvmm_set_page_table(PTEFRAME(__frames[i].words[j]));
				return (i*256) + j;
			}
		}
	}
	printf("(RETIRAR ESTE PRINT) NÃO foram encontradas as informações da TABELA 1 do PROCESSO %i NA TABELA DE SISTEMA.\n", id_processos);
	printf("(RETIRAR ESTE PRINT) Procurando por uma linha livre na TABELA DE SISTEMA para alocar A TABELA 1 DO PROCESSO %i...\n", id_processos);
	for(i=0x0; i<16; i++)
	{
		(i==0) ? (j=INICIO_TABELA_SISTEMA) : (j=0);
		for(; j<=FIM_TABELA_SISTEMA ; j++)
		{
			if(__frames[i].words[j] == 0x0)
			{
				printf("(RETIRAR ESTE PRINT) OS DADOS DA TABELA 1 DO PROCESSO %i serão inseridos na linha 0X%X DA TABELA DE SISTEMA\n", id_processos, (i*256) + j);
				__frames[i].words[j] = (id_processos << 24);
				dccvmm_set_page_table(0x0);
				return (i*256) + j;
			}
		}
	}
	dccvmm_set_page_table(0x0);
	return 0x0;
}

uint32_t dump_setor_livre (uint32_t frame) {
    int i, j, k;

    for (i = 0; i < 0x80; i++) {
        dccvmm_load_frame(i, 0x1);
        for (j = 0; j < 0x100; j++) {
            for (k = 0; k < 0x20; k++) {
                if (!(__frames[i].words[j] & (0x00000001 << k))) { // Setor livre
                    dccvmm_dump_frame(frame, k + (j * 0x20) + (i * 0x20 * 0x100));
                    return k + (j * 0x20) + (i * 0x20 * 0x100);
                }
            }
        }
    }
    printf("Erro: Acabou o espaço em disco.\n");
    return VM_ABORT;
}

uint32_t restaurar_setor (uint32_t setor, uint32_t frame) {
    dccvmm_load_frame(setor, frame);

    uint32_t i, j, k;
    i = setor / (0x20 * 0x100); // Frame
    j = (setor % (0x20 * 0x100)) / 0x20 // Word offset
    k = setor % 0x20; // Bit offset

    uint32_t mask = ~(0x00000001 << k);

    dccvmm_load_frame(i, 0x1); // Pull usage frame

    __frames[1].words[j] &= mask; // Set unused

    dccvmm_dump_frame(0x1, i); // Push updated usage
}