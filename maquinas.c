#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <stdbool.h>

#define PORT 25555
char ip[] = "127.0.0.1";

struct carta {

	int numero; // 1 (às) - 13 (rei)
	char naipe; // O = ouros, E = espada, C = copas, P = paus
};

struct package {

	char type; // J = jogada, C = cartas recebidas, V = verificar quem tem 2 de paus
	int origin;
	int src;
	int dst;
	short len;
	int proxMao;
	char data[256];
	bool token;
	
};

struct jogador {

    	int id;  // número ou identificador do jogador
    	int pontos;  // pontuação acumulada
    	struct carta mao[13];  // mão com até 13 cartas
    	struct carta cartasAcumuladas[52];
	bool copasQuebrado;
  	bool vencedor;
   	bool primeiraRodada; 
   	bool perdi;
};

///////////////////////////////////////////////////////////////////////////////////////////

void error(const char *msg);
//void montaPacoteJogada(struct package *p, char type, int src, int dst, int valor, char naipe, int node, struct package *pRecebido);
void montaPacoteCartas(struct carta *baralho, struct package *p, int src, int dst, int node);
void receive_message(int sockfd, struct package *p);
void send_message(int sockfd, struct package *p, struct sockaddr_in *prox_addr);
void gera_baralho(struct carta *baralho);
void embaralhar(struct carta *baralho);
void enviaToken(int sockfd, int node, struct sockaddr_in *prox_addr, int dst);
void defineMao(struct package *p, struct jogador *eu);
void iniciaJogador(struct jogador *eu, int node);
void mostraMao(struct jogador *eu);
//   realiza todas as acoes iniciais de inicio de rodada
void iniciaRodada(struct carta *baralho, struct package *castasPackage, int node, int dst);
void montaPacoteJogada(struct jogador *eu, int idEscolhido, struct package *p, int src, int dst, int node, struct package *pRecebido);
void mostraRodadaAtual(struct package *pRecebido);
void removeCartaMao(struct jogador *eu, int index);
bool verifica2paus(struct jogador *eu, int *index);
void montaPacoteVerificacao(struct package *p, int src, int dst);
void montaPacoteFimPrimeiraRodada(struct package *p, struct jogador *eu);
void montaPacoteFimDeJogo(struct package *p, struct jogador *eu, bool primeiro);

void mostraPlacar(struct package *pacoteRecebido){

    	int ids[4];
    	int pontos[4];
	
    	for (int i = 0; i < 4; ++i) {
    	
    		ids[i] = pacoteRecebido->data[i * 4] - '0';

		char pontos_str[4];
		pontos_str[0] = pacoteRecebido->data[i * 4 + 1];
		pontos_str[1] = pacoteRecebido->data[i * 4 + 2];
		pontos_str[2] = pacoteRecebido->data[i * 4 + 3];
		pontos_str[3] = '\0';

		pontos[i] = atoi(pontos_str);
   	}

    // Ordenação por pontuação decrescente
    	for (int i = 0; i < 4 - 1; i++) {
		for (int j = i + 1; j < 4; j++) {
		    if (pontos[i] < pontos[j]) {
		        // troca pontos
		        int temp_pontos = pontos[i];
		        pontos[i] = pontos[j];
		        pontos[j] = temp_pontos;

		        // troca ids também
		        int temp_id = ids[i];
		        ids[i] = ids[j];
		        ids[j] = temp_id;
		    }
		}
   	}

    // Exibe o placar
    	
    	int k = 1;
    
    	for (int i = 3; i >= 0; --i) {
		if (i == 0) {
		    printf("Perdedor: Player %d (pontos: %d)\n", ids[i], pontos[i]);
		} else {
		    printf("%dº lugar: Player %d (pontos: %d)\n", k, ids[i], pontos[i]);
		    ++k;
		}
    	}


}

bool checaValidadeCartaJogada(struct package *pacoteRecebido, struct jogador *eu, int indexEscolhido){
	
	bool temCartaDoNaipe = false;
	
	for(int i = 0; i < 13; ++i){
	
		if(eu->mao[i].naipe == pacoteRecebido->data[3])
			temCartaDoNaipe = true;
	}


	if( (eu->mao[indexEscolhido].naipe != pacoteRecebido->data[3] && temCartaDoNaipe) || (eu->mao[indexEscolhido].numero == -1 || (indexEscolhido < 0 || indexEscolhido > 12)) )
		return false;
		
	return true;

}

int verificaQuemGanhouRodada(struct package *pacoteRecebido, struct jogador *eu){

	printf("pacote recebido para verificar quem ganhou: %s\n", pacoteRecebido->data);

	char naipeDaRodada = pacoteRecebido->data[3];
	int maiorValor = -1;
	int vencedor = -1;
	
	for (int i = 0; i < 4; i++) {
        	int offset = i * 4;
        	
        	// extrai valor e naipe da carta i
        	char valor_str[3];
        	valor_str[0] = pacoteRecebido->data[offset + 1];
        	valor_str[1] = pacoteRecebido->data[offset + 2];
        	valor_str[2] = '\0';
        
        	int valor = atoi(valor_str);
        	char naipe = pacoteRecebido->data[offset + 3];
	
        	// verifica se carta segue o naipe da rodada e é maior que a atual vencedora
        	if (naipe == naipeDaRodada) {
        		int forca = (valor == 1) ? 14 : valor;
        		if(forca > maiorValor){
        			
        			//printf("\033[1matual mais forte: %d id: %d\033[0m\n", maiorValor, vencedor);
        		
        	    		maiorValor = forca;	
        	    		
        	    		char winner[2];
        	    		winner[0] = pacoteRecebido->data[offset];
        	    		winner[1] = '\0';
        	    		
        	    		vencedor = atoi(winner); 
        	    		
        	    	}
        	}
    	}
	
	return vencedor;

}



void montaPacoteVencedor(struct package *pacoteRecebido, int vencedor, struct package *pacoteDoVencedor, struct jogador *eu){

	memset(pacoteDoVencedor, 0, sizeof(*pacoteDoVencedor));

	pacoteDoVencedor->src = eu->id;
	pacoteDoVencedor->dst = vencedor;
	pacoteDoVencedor->type = 'W';
	pacoteDoVencedor->token = true;
	
	//printf("data pacote recebido: %s\n", pacoteRecebido->data);
	
	strncpy(pacoteDoVencedor->data, pacoteRecebido->data, 16);

	//printf("data pacote do vencedor: %s\n", pacoteDoVencedor->data);

	pacoteDoVencedor->len = 16;

}

void atribuiPontos(struct package *cards, struct jogador *eu){

	for (int i = 0; i < 4; ++i){
	
		if (cards->data[i*4 + 3] == 'C')
			eu->pontos += 1;
			
		if (cards->data[i*4 + 3] == 'E' && cards->data[i*4 + 1] == '1' && cards->data[i*4 + 2] == '2')
			eu->pontos += 12;
	}

}

void montaPacoteCopasQuebrado(struct package *p, struct jogador *eu){

	memset(p, 0, sizeof(*p));

	p->src = eu->id;
	p->dst = 0;
	p->type = 'I';

}

///////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[]){

	//char *meuip = argv[1];
	//char *proxip = argv[2];
	
	int node = atoi(argv[1]);
	int dstnode;
	int sockfd;
	
	struct sockaddr_in meu_addr, prox_addr;
    	struct package p;

	// conexao com outras maquinas do dinf

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    	if (sockfd < 0) error("socket");
	
	////
	int minha_porta = atoi(argv[2]);      // porta do próprio nó
	int prox_porta = atoi(argv[3]);       // porta do próximo nó
	
	////
	
	memset(&meu_addr, 0, sizeof(meu_addr));
    	meu_addr.sin_family = AF_INET;
    	//meu_addr.sin_port = htons(PORT); 
    	meu_addr.sin_port = htons(minha_porta); 	
    	inet_pton(AF_INET, ip, &meu_addr.sin_addr);
	//inet_pton(AF_INET, meuip, &meu_addr.sin_addr);

	if (bind(sockfd, (struct sockaddr *)&meu_addr, sizeof(meu_addr)) < 0)	error("bind");

	memset(&prox_addr, 0, sizeof(prox_addr));
    	prox_addr.sin_family = AF_INET;
    	//prox_addr.sin_port = htons(PORT);
    	prox_addr.sin_port = htons(prox_porta);
    	inet_pton(AF_INET, ip, &prox_addr.sin_addr);
	//inet_pton(AF_INET, proxip, &prox_addr.sin_addr);

	if(node == 4)
		dstnode = 1;
	else
		dstnode = node + 1;

	//////////////   logica do jogo   ////////////////////
   	
	srand(time(NULL));
	struct carta *baralho = malloc(52*(sizeof(struct carta)));
	struct jogador *eu = malloc( sizeof(struct jogador) );
	struct package cartasPackage;
	struct package pacoteRecebido;
	struct package cartasRodada;
	struct package verifica2P;
	struct package pacoteDoVencedor;
	struct package pacoteFimDeJogo;
	
	iniciaJogador(eu, node);
	int cartaEscolhida;
	int index;
	bool encerrar = false;	
	//   jogador 1 eh o primeiro a jogar
	if(eu->id == 1){
	
		iniciaRodada(baralho, &cartasPackage, node, dstnode); /* cartasPackage ja contem o pacote com
		todas as cartas embaralhadas */
			
		defineMao(&cartasPackage, eu);
		
		send_message(sockfd, &cartasPackage, &prox_addr);
		
		eu->vencedor = true;
	}

	while(1){
		
		receive_message(sockfd, &pacoteRecebido);
		
		fflush(stdout);
		//printf("oi1\n");
		
		if(pacoteRecebido.src == node && pacoteRecebido.type != 'W'){
		
			printf("mensagem deu a volta no anel!\n\n");

			if(eu->primeiraRodada && pacoteRecebido.type == 'C'){
			
				printf("Verificando se possuo o dois de paus...");
			
				if( verifica2paus(eu, &index) ){
					printf("\nEstou com o 2 de paus!\n");
					montaPacoteJogada(eu, index, &cartasRodada, node, dstnode, node, NULL);
					removeCartaMao(eu, index);
					send_message(sockfd, &cartasRodada, &prox_addr);
				}else{
					montaPacoteVerificacao(&verifica2P, node, dstnode);
					printf("\nNao estou com o 2 de paus!\n");
					send_message(sockfd, &verifica2P, &prox_addr);
				}
			
				eu->vencedor = false;		
			}
			
			if(!eu->primeiraRodada && pacoteRecebido.type == 'C'){
			
				printf("Eu inicio a rodada atual!\nEscolha uma carta:\n");
				mostraMao(eu);
				int idEscolhido;
				
				printf("\033[1mo que tem no id escolhido:\033[0m\n");
				
				while(!checaValidadeCartaJogada(&pacoteRecebido, eu, idEscolhido)){
					//system("clear");
					printf("\nOops! A carta que voce jogou eh invalida! jogue outra correspondente ao naipe da carta inicial\n");
					mostraMao(eu);
					mostraRodadaAtual(&pacoteRecebido);
					scanf("%d", &idEscolhido);
				}
				
				montaPacoteJogada(eu, idEscolhido, &cartasRodada, node, dstnode, node, NULL);
				send_message(sockfd, &cartasRodada, &prox_addr);
			}
			
			if(pacoteRecebido.type == 'E'){
				mostraPlacar(&pacoteRecebido);
				encerrar = true;
			}
			
			
			
		}else if(pacoteRecebido.dst == 0){
		
			/*if(pacoteRecebido.type = 'I'){
			
				eu->copasQuebrado = true;
				send_message(sockfd, &pacoteRecebido, &prox_addr);
			}*/
			
		
			if(pacoteRecebido.type == 'V' && pacoteRecebido.dst == 0 && pacoteRecebido.data[0] == 'a' && pacoteRecebido.data[1] == 'a'){
				eu->primeiraRodada = false;
				send_message(sockfd, &pacoteRecebido, &prox_addr);
			}
			
	/*////////*/		if(!eu->vencedor && pacoteRecebido.type == 'C'){
				defineMao(&pacoteRecebido, eu);
				mostraMao(eu);
				send_message(sockfd, &pacoteRecebido, &prox_addr);
				
				
			}			
		
			if(pacoteRecebido.type == 'E'){
				mostraPlacar(&pacoteRecebido);
				send_message(sockfd, &pacoteRecebido, &prox_addr);
				encerrar = true;
			}
		
		}else if(pacoteRecebido.dst != node){
		
			printf("\nnao sou o destinatario! repassando mensagem...\n");
			
			send_message(sockfd, &pacoteRecebido, &prox_addr);		
		
		}else{
		
			//printf("\noi2\n");
			switch(pacoteRecebido.type){
					
				case 'J':
				
					//system("clear");
					
					//printf("\ntamanho do pacote recebido: %d\n", pacoteRecebido.len);
					
					if(pacoteRecebido.len >= 16){
						printf("Rodada encerrada!\n\n");
				
						montaPacoteVencedor(&pacoteRecebido, verificaQuemGanhouRodada(&pacoteRecebido, eu), &pacoteDoVencedor, eu);
						send_message(sockfd, &pacoteDoVencedor, &prox_addr);
						break;
					}
					
					mostraRodadaAtual(&pacoteRecebido);
					mostraMao(eu);		
					printf("Sua vez! Escolha qual carta ira jogar!\n");
					printf("naipe da rodada: %c\n", pacoteRecebido.data[3]);			
					int idEscolhido;
					scanf("%d", &idEscolhido);
					
					while(!checaValidadeCartaJogada(&pacoteRecebido, eu, idEscolhido)){
						//system("clear");
						printf("\nOops! A carta que voce jogou eh invalida! jogue outra correspondente ao naipe da carta inicial\n");
						mostraMao(eu);
						mostraRodadaAtual(&pacoteRecebido);
						scanf("%d", &idEscolhido);
					}
					
					/*if(pacoteRecebido.data[3] != 'C' && eu->mao[idEscolhido].naipe == 'C'){
					
						printf("Copas quebrado!\nComunicando a rede...\n\n");
						struct package aux;
						montaPacoteCopasQuebrado(&aux, eu);
						send_message(sockfd, &aux, &prox_addr);
						receive_message(sockfd, &pacoteRecebido);
						printf("Todo mundo da rede viu que o copas foi quebrado!!\n\n");
						eu->copasQuebrado = true;
					}*/
					
					montaPacoteJogada(eu, idEscolhido, &cartasRodada, node, dstnode, node, &pacoteRecebido);
					removeCartaMao(eu, idEscolhido);
					send_message(sockfd, &cartasRodada, &prox_addr);
					
				break;
				
				case 'V':
						
				
					if( verifica2paus(eu, &index) && eu->primeiraRodada){
						printf("\nEstou com o 2 de paus!\n");
						montaPacoteJogada(eu, index, &cartasRodada, node, dstnode, node, NULL);
						removeCartaMao(eu, index);
						send_message(sockfd, &cartasRodada, &prox_addr);
					}else if(!verifica2paus(eu, &index) && eu->primeiraRodada){
						montaPacoteVerificacao(&verifica2P, node, dstnode);
						printf("\nNao estou com o 2 de paus! passando para o proximo...\n");
						send_message(sockfd, &verifica2P, &prox_addr);
					}
				
				break;
				
				case 'W':
				
					if(eu->primeiraRodada){
						
						montaPacoteFimPrimeiraRodada(&verifica2P, eu);
						eu->primeiraRodada = false;		
						send_message(sockfd, &verifica2P, &prox_addr);
						receive_message(sockfd, &pacoteRecebido);
					}
				
					atribuiPontos(&pacoteRecebido, eu);
					
					if(eu->pontos >= 100){  //logica para encerrar o jogo
					
						printf("A partida acabou! E eu perdi hahaha\n");
						montaPacoteFimDeJogo(&pacoteFimDeJogo, eu, true);
						eu->perdi = true;
						send_message(sockfd, &pacoteFimDeJogo, &prox_addr);
						
					}else{
					
						printf("\nVenci a rodada!\n");
						printf("\nMeus pontos: %d\n", eu->pontos);
						
						memset(&pacoteRecebido, 0, sizeof(pacoteRecebido));
						
						eu->vencedor = true;
						
						printf("Eu inicio a rodada atual!\nEscolha uma carta:\n");
						mostraMao(eu);
						
						scanf("%d", &idEscolhido);
						
						while(!checaValidadeCartaJogada(&pacoteRecebido, eu, idEscolhido)){
							//system("clear");
							printf("\nOops! A carta que voce jogou eh invalida! jogue outra correspondente ao naipe da carta inicial\n");
							mostraMao(eu);
							mostraRodadaAtual(&pacoteRecebido);
							scanf("%d", &idEscolhido);
						}
						
						montaPacoteJogada(eu, idEscolhido, &cartasRodada, node, dstnode, node, NULL);
						removeCartaMao(eu, idEscolhido);
						send_message(sockfd, &cartasRodada, &prox_addr);
					}
				
				break;
				
				case 'E':
				
					if(eu->perdi){
					
						pacoteRecebido.src = eu->id;
						pacoteRecebido.dst = 0;
						send_message(sockfd, &pacoteRecebido, &prox_addr);
						
						break;
					}
				
					printf("pacote do fim do jogo: %s\n", pacoteRecebido.data);
					montaPacoteFimDeJogo(&pacoteRecebido, eu, false);
					send_message(sockfd, &pacoteRecebido, &prox_addr);
				break;
				
			}
			
			
			//send_message(sockfd, &pacoteRecebido, &prox_addr);
			
			//printf("\noi3\n");
		}
		
		//printf("\noi4\n");
		
		if(encerrar) break;
			
	}
    	
	free(baralho);
	free(eu);
	close(sockfd);
	return 0;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void montaPacoteFimPrimeiraRodada(struct package *p, struct jogador *eu){

	memset(p, 0, sizeof(*p));

	p->type = 'V';
	p->src = eu->id;
	p->dst = 0;
	strcpy(p->data, "aa");
}

void iniciaRodada(struct carta *baralho, struct package *cartasPackage, int node, int dst){

	//system("clear");	
	printf("Distribuindo cartas...\n");
	gera_baralho(baralho);
	embaralhar(baralho);

	montaPacoteCartas(baralho, cartasPackage, node, 0, node);
	
}


void error(const char *msg){

	perror(msg);
	exit(1);
}

void montaPacoteCartas(struct carta *baralho, struct package *p, int src, int dst, int node){

	memset(p, 0, sizeof(*p));

	p->type = 'C';
	p->src = src;
	p->dst = dst;
	p->origin = node;
	p->token = true;
	
	int k = 0;
	
	for (int i = 0; i < 52; i++) {
        	sprintf(&p->data[k], "%02d%c", baralho[i].numero, baralho[i].naipe);
        	k += 3;
    	}

	p->len = k;
}

void receive_message(int sockfd, struct package *p) {

    	ssize_t n = recvfrom(sockfd, p, sizeof(*p), 0, NULL, NULL);
    	if (n < 0) {
        	perror("Erro ao receber pacote");
    	} else {
        	printf("\nMensagem recebida: Tipo=%c, De=%d, Para=%d, Dados=%s\n",
               	p->type, p->src, p->dst, p->data);
    	}
}

void send_message(int sockfd, struct package *p, struct sockaddr_in *prox_addr) {

    	ssize_t sent = sendto(sockfd, p, sizeof(*p), 0, (struct sockaddr *)prox_addr, sizeof(*prox_addr));
    	if (sent < 0) {
        	perror("Erro ao enviar pacote");
    	} else {
        	printf("\nMensagem enviada: Tipo=%c, De=%d, Para=%d, Dados=%s\n", p->type, p->src, p->dst, p->data);
    	}
    	
}

void gera_baralho(struct carta *baralho){

	char naipes[] = {'C', 'E', 'O', 'P'};
	int k = 0;

	for(int i = 0; i < 4; ++i){
		for(int v = 1; v <= 13; ++v){
			
			baralho[k].numero = v;
			baralho[k].naipe = naipes[i];
			++k;
		}
	}


}

void embaralhar(struct carta *baralho){

	for (int i = 51; i > 0; i--) {
        	int j = rand() % (i + 1);  // sorteia entre 0 e i
        	struct carta temp = baralho[i];
        	baralho[i] = baralho[j];
        	baralho[j] = temp;
    	}	

}

void enviaToken(int sockfd, int node, struct sockaddr_in *prox_addr, int dst){

	struct package token;
    	//montaPacoteJogada(&token, 'T', node, dst, 0, 'X', node);
    	send_message(sockfd, &token, prox_addr);

}

void defineMao(struct package *p, struct jogador *eu){

	int inicio = (eu->id - 1)* 13 * 3;

	for(int i = 0; i < 13; ++i){
	
		char numero_str[3];
		numero_str[0] = p->data[inicio + i*3];
		numero_str[1] = p->data[inicio + i*3 + 1];
		numero_str[2] = '\0';
		
		eu->mao[i].numero = atoi(numero_str);
		eu->mao[i].naipe = p->data[inicio + i*3 +2];
		
		
	}
	
}

void iniciaJogador(struct jogador *eu, int node){

	eu->id = node;
	eu->pontos = 0;
	eu->vencedor = false;
	eu->copasQuebrado = false;
	eu->primeiraRodada = true;
	eu->perdi = false;
	
	if(eu->id == 1) eu->vencedor = true;
}

void mostraMao(struct jogador *eu) {
	printf("\nSua mão:\n");

    	for (int i = 0; i < 13; ++i) {
    
    	if (eu->mao[i].numero == -1) continue;
    
        char *simboloNaipe;
	char simboloNumero[3];
	
	switch (eu->mao[i].numero) {
       		case 1:  strcpy(simboloNumero, "A"); break;  // Ás
            	case 11: strcpy(simboloNumero, "J"); break;  // Valete
            	case 12: strcpy(simboloNumero, "Q"); break;  // Dama
            	case 13: strcpy(simboloNumero, "K"); break;  // Rei
            	default: snprintf(simboloNumero, sizeof(simboloNumero), "%02d", eu->mao[i].numero); break;
        }
        
        switch (eu->mao[i].naipe) {
            	case 'C': simboloNaipe = "♥"; break; // Copas ♥
            	case 'E': simboloNaipe = "♠"; break; // Espadas ♠
            	case 'O': simboloNaipe = "♦"; break; // Ouros ♦
            	case 'P': simboloNaipe = "♣";break; // Paus ♣
            	default:  simboloNaipe = "?"; break;
        }

        printf("[%d] %2s%s\n", i, simboloNumero, simboloNaipe);
    	}

}

void mostraRodadaAtual(struct package *pRecebido){

	char simboloNumero[3];
	
	for(int i = 0; i < strlen(pRecebido->data); i += 4){
	
		simboloNumero[0] = pRecebido->data[i+1];
		simboloNumero[1] = pRecebido->data[i+2];
		simboloNumero[2] = '\0';
		 
		printf("Jogador %c jogou: %02d%c\n", pRecebido->data[i], atoi(simboloNumero), pRecebido->data[i+3]);
	
	
	}
}

void montaPacoteFimDeJogo(struct package *p, struct jogador *eu, bool primeiro){
	
	char IDePontuacao[16];
	snprintf(IDePontuacao, sizeof(IDePontuacao), "%d%03d", eu->id, eu->pontos);


	size_t prevLen = strlen(p->data);
    	
    	
    	if(primeiro) memset(p->data, 0, sizeof(p->data));

    	strcat(p->data, IDePontuacao);

    	
    	p->type = 'E';
	p->src = eu->id;
	p->dst = (eu->id == 4) ? 1 : eu->id + 1;

	p->len = strlen(p->data);

}


void montaPacoteJogada(struct jogador *eu, int idEscolhido, struct package *p, int src, int dst, int node, struct package *pRecebido){

	memset(p, 0, sizeof(*p));
	
	char novaJogada[16];  
   	snprintf(novaJogada, sizeof(novaJogada), "%d%02d%c", node, eu->mao[idEscolhido].numero, eu->mao[idEscolhido].naipe);
	
	if (pRecebido) {
        	size_t prevLen = strlen(pRecebido->data);
        	strncpy(p->data, pRecebido->data, sizeof(p->data) - 1);
        	strncat(p->data, novaJogada, sizeof(p->data) - 1 - prevLen);
   	} else {
        	strncpy(p->data, novaJogada, sizeof(p->data) - 1);
    	}
	
	/*printf("Pacote formado: %s\n\n", p->data);
	printf("Tamanho do pacotao: %ld", strlen(p->data));*/
	
	p->type = 'J';
	p->src = src;
	p->dst = dst;
	//p->proxMao = 2;
	p->origin = node;
	p->token = true;
	

	p->len = strlen(p->data);
}

bool verifica2paus(struct jogador *eu, int *index){

	for(int i = 0; i < 13; ++i){
	
		*index = i;
	
		if(eu->mao[i].numero == 2 && eu->mao[i].naipe == 'P')
			return true;
	
	}

	return false;

}

void removeCartaMao(struct jogador *eu, int index){

	eu->mao[index].numero = -1;
	eu->mao[index].naipe = 'N';
}

void montaPacoteVerificacao(struct package *p, int src, int dst){

	memset(p, 0, sizeof(*p));

	p->type = 'V';
	p->src = src;
	p->dst = dst;
	p->len = 0;
	
}



