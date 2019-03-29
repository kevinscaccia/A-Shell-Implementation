/*************************************** 
 * Includes
 ***************************************
 ***************************************/
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>
/*************************************** 
 * Constantes
 ***************************************
 ***************************************/
#define LINE_BUFFER_SIZE 100
#define SHELL_STATUS_CLOSE 0
#define SHELL_STATUS_CONTINUE 1
#define MAX_COMMAND_BY_LINE 30
#define MAX_COMMAND_SIZE 100
#define TRUE 1
#define FALSE 0
#define READ_END 0
#define WRITE_END 1
int GLOBAL_PIPE[2];
int GLOBAL_PIPE2[2];
int MULTIPLE_PIPES = FALSE;
int MULTIPLE_PIPES_FLAG = FALSE;
/*************************************** 
 * Protótipos
 ***************************************
 ***************************************/
//Ciclo de vida do Shell
void loop_shell();
void start_shell();
void finish_shell();
// Manipulação e Execução dos Comandos
char** split_commands(char* line);
int execute_commands(char* line);
char** get_command_parameters(char** matrix, int* index);
// Funções e Procedimentos Auxiliares
void print_commands(char** matrix);
void read_line(char* buffer);
int is_operador(char* c);
int is_operador_char(char c);
int is_command(char* c);
void print_error(char* c);
void print_alert(char* c);
void printf_alert(char* format, char* c);
void fechar_pipes_abertos();
/*************************************** 
 * Main
 ***************************************
 ***************************************/
int main(int argc, char** argv){
	start_shell();
	loop_shell();
	finish_shell();
	return 0;
}
/*************************************** 
 * Ciclo de vida do Shell
 ***************************************
 ***************************************/
void start_shell(){
	printf("\033[0;1m"); // white bold
	printf("---------------\033[1;32m K-Shell\033[0;1m ---------------");
	printf("\n---------------------------------------\n");
}
void loop_shell(){
	// Buffer para leitura da linha
	char *line;
	int status; // status dos comandos
	do{
		line = (char*) malloc(sizeof(char)*LINE_BUFFER_SIZE);
		// lê comando da entrada padrão
		printf("\033[0;1m >> \033[0m");
		read_line(line);// passa o ponteiro da linha	
		// parseia os comandos em uma matriz de comandos
		status = execute_commands(line);
		free(line);
	}while(status == SHELL_STATUS_CONTINUE);

}
void finish_shell(){
	printf("\033[0;1m----------------------------------\033[1;31mbye\033[0;1m..\n");
}
/*************************************** 
 * Manipulação dos Comandos
 ***************************************
 ***************************************/

char** split_commands(char* line){
		char ** commands = (char**) malloc(sizeof(char*)*MAX_COMMAND_BY_LINE*MAX_COMMAND_SIZE);
	int i = 0;
	char *p = strtok(line, " ");
	while(p != NULL) {
		commands[i++] = p;
    	p = strtok(NULL, " ");
	}
	return commands;
}
/*************************************** 
 * Execução dos Comandos
 ***************************************
 ***************************************/
void execute_pipe_operand(char* commands){
	print_alert("Pipe Operator detected!");
	// comandos separados por um pipe
	char* command_0 = commands;
	int i = 0;
	while(commands[i] != '|')// encontra o pipe
		i++;
	char* command_1 = &commands[i+1];// pega o comando(depois do pipe)
	printf("PIPE: Comando 0: %s, Comando 1: %s",command_0, command_1);
}
char** get_command_parameters(char** matrix, int* index){
	char** args = (char**) malloc(sizeof(char*)*MAX_COMMAND_SIZE);
	int i = *index;  // le argumentos a partir do comando atual
	// inclui o nome do comando como argumento
	int ii = 0;
	//printf("Comando na posicao <%d> da matriz de comandos\n",i);
	char* arg = matrix[i];  // argumento atual
	while(arg != NULL && !is_operador(arg)){
		//printf("Adicionando <%s> como argumento \n", arg);
		args[ii] = matrix[i];
		arg = matrix[++i];
		ii++;
	}
	args[i] = NULL;
	*index = i;
	return args;
}
int sem_operador(char** commands, int* index){
	pid_t filho;
	char** com_mat = get_command_parameters(commands, index);
	//print_commands(com_mat);
	filho = fork();
	if(filho == 0){
		if(execvp(com_mat[0], com_mat) < 0){
				print_error("Comando possívelmente inválido ou incompleto:");
				perror("->");
				return SHELL_STATUS_CONTINUE;
			}else{
				printf("Executado corretamente");
			}
	}else{
		waitpid(filho, NULL, 0);
	}
	return SHELL_STATUS_CONTINUE;
}
int operador_depois(char** commands, int* index){
	pid_t filho;
	// resolver operador
	if(TRUE){//commands[*index+1][0] == '|'){
		print_alert("Primeiro Comando");
		char** com_mat = get_command_parameters(commands, index);
		// Cria pipe pro acesso // primeiro pipe
		if(pipe(GLOBAL_PIPE) < 0){
			print_error("Erro ao criar PIPE 1");
		}else{
			filho = fork();
			if(filho == 0){  // Processo Filho
				close(GLOBAL_PIPE[READ_END]);// FILHO nao lê no PIPE 1
				dup2(GLOBAL_PIPE[WRITE_END], STDOUT_FILENO); // FILHO ESCREVE NO PIPE 1
				// substitui espaço de endereçamento (comando atual[i])
				if(execvp(com_mat[0], com_mat) < 0){
					print_error("Comando possívelmente inválido ou incompleto:");
					perror("->");
					return SHELL_STATUS_CONTINUE;
				}
			}else{
                close(GLOBAL_PIPE[WRITE_END]);// PAI NAO ESCREVE NO PIPE 1 MAS PRECISA LER DELE (FILHO)
				waitpid(filho, NULL, 0);
				print_alert("Primeiro filho terminou");
				
			}
			// Redireciona saida do commando atual
			// para o pipe, para posterior uso
		}
	}
	return SHELL_STATUS_CONTINUE;
}
int operador_antes(char** commands, int* index){
	pid_t filho;
	if(TRUE){//commands[*index-1][0] == '|'){
		// Como existe operador antes, o pipe ja foi criado
		char** com_mat = get_command_parameters(commands, index);// ANTES DO FORK

		filho = fork();
		if(filho == 0){
		
		    if(MULTIPLE_PIPES){  // mais de 2 pipes: LÊ de um dos dois
		    	if(!MULTIPLE_PIPES_FLAG){// NUMERO IMPAR DE COMANDOS INTERNOS
		    		dup2(GLOBAL_PIPE2[READ_END], STDIN_FILENO); // LE DO PIPE 2		
			    	print_alert("Eu deveria escrever na saida padrao");
		    	}else{ // numero par de pipes internos
		    		dup2(GLOBAL_PIPE[READ_END], STDIN_FILENO); // LE DO PIPE 1	
			    	print_alert("Eu deveria escrever na saida padrao");
		    	}
            }else{  // um unico pipe, LE do pipe 1
                print_alert("Ultimo comando | um unico pipe");
			    dup2(GLOBAL_PIPE[READ_END], STDIN_FILENO); // LE DO PIPE 1
            }
			if(execvp(com_mat[0], com_mat) < 0){
				print_error("Comando possívelmente inválido ou incompleto:");
				perror("->");
				//return SHELL_STATUS_CONTINUE;
			}
		}else{
		    if(MULTIPLE_PIPES){  // mais de 1 pipe: close nos dois
                close(GLOBAL_PIPE[READ_END]);// fecha pipe 1 para 															o pai
                close(GLOBAL_PIPE[WRITE_END]);// fecha pipe 1 para o pai
                close(GLOBAL_PIPE2[READ_END]);// fecha pipe 2 para o pai
                close(GLOBAL_PIPE2[WRITE_END]);// fecha pipe 2 para o pai
            }else{  // um unico pipe: close em um só
                close(GLOBAL_PIPE[READ_END]);// PAI NAO LÊ // pipe nao existe mais
            }
			waitpid(filho, NULL, 0);
			print_alert("Ultimo filho terminou");
		}		
		
	}																																							
	return SHELL_STATUS_CONTINUE;
}

int operador_antes_e_depois(char** commands, int* index){
	pid_t filho;
	if(TRUE){//commands[*index-1][0] == '|'){
		// Como existe operador antes, o pipe ja foi criado
		char** com_mat = get_command_parameters(commands, index);// ANTES DO FORK
		
		if(!MULTIPLE_PIPES){
		    // primeiro comando 'do meio' criado
		    // cria o segundo pipe auxiliar
		    
		    if(MULTIPLE_PIPES_FLAG){
		    	
		    }else{// cria o pipe 2
		    	if(pipe(GLOBAL_PIPE2) < 0)
		        	return -1;
		    }
		    
		}
		
		filho = fork();
		// ja tem um pipe criado (GLOBAL_PIPE) para leitura,
		// pai tem acesso a leitura no pipe1
		if(filho == 0){
		    
		    if(!MULTIPLE_PIPES){ // primeiro comando 'do meio' criado, filho lê do pipe 1 e escreve no 2
               print_alert("Primeiro comando do meio");
               dup2(GLOBAL_PIPE[READ_END], STDIN_FILENO); // lê do PIPE 1
               dup2(GLOBAL_PIPE2[WRITE_END], STDOUT_FILENO); // escreve no PIPE 2
            }else{  // mais de 2 pipes
                print_alert("Comando do meio: mais de dois pipes");
            }
			if(execvp(com_mat[0], com_mat) < 0){
				print_error("Comando possívelmente inválido ou incompleto:");
				perror("->");
				//return SHELL_STATUS_CONTINUE;
			}
		}else{ // codigo do pai
		    if(MULTIPLE_PIPES){ // mais de 2 pipes,
                
            }else{  // um unico filho do meio. pai fecha a escrita no pipe 2 e a leitura do pipe1
                close(GLOBAL_PIPE[READ_END]);// fecha pipe LEITURA 1 para o pai // pipe 1 nao 'existe' mais
                close(GLOBAL_PIPE2[WRITE_END]);// fecha ESCRITA pipe 2 para o pai
            }
			waitpid(filho, NULL, 0);
			MULTIPLE_PIPES = TRUE; // indica que ja foi criado o pipe 2
			print_alert("Filho do meio terminou");
			return SHELL_STATUS_CONTINUE;
		}		
		return SHELL_STATUS_CONTINUE;
	}else{
		return SHELL_STATUS_CONTINUE;
	}
}
/*
			char* inbuf = (char*)malloc(sizeof(char)*1000);
        		while(read(GLOBAL_PIPE[READ_END], inbuf, 1000))
        			printf("CONTEUDO: %s \n", inbuf); 
  

			//dup2(GLOBAL_PIPE_AUX[WRITE_END], STDOUT_FILENO); 
*/
int ha_operador_antes(char** commands, int index){
	if(index <= 0)
		return FALSE;
	char* cmd = commands[index];
	while(cmd != NULL && index >= 0){
		if(is_operador(cmd))
			return TRUE;// ha operador depois
		cmd = commands[--index];
	}
	return FALSE;
}
int ha_operador_depois(char** commands, int index){
	char* cmd = commands[index];
	while(cmd != NULL){
		if(is_operador(cmd))
			return TRUE;// ha operador depois
		cmd = commands[++index];
	}
	return FALSE;
}
int execute_commands(char* line){
	char** commands = split_commands(line);
	//print_commands(commands);
	int i = 0;
	while(commands[i] != NULL){
		if(is_command(commands[i])){  // encontrou um COMANDO
			if(ha_operador_antes(commands, i)){
				if(ha_operador_depois(commands, i)){
					print_alert("Ha operador antes e depois");
					operador_antes_e_depois(commands, &i);
				}else{
					print_alert("Ha operador antes");
					operador_antes(commands, &i);
				}
			}else if(ha_operador_depois(commands, i)){
				print_alert("Ha operador depois");
				operador_depois(commands, &i);
			}else{
				print_alert("Nao ha operador");
				sem_operador(commands, &i);
			}
		}
		i++;
	}
	fechar_pipes_abertos();
	MULTIPLE_PIPES = FALSE;
	printf("\033[0m");
	return SHELL_STATUS_CONTINUE;
}
void fechar_pipes_abertos(){
	if(GLOBAL_PIPE[WRITE_END] != 0)
		close(GLOBAL_PIPE[WRITE_END]);
		
	if(GLOBAL_PIPE[READ_END] != 0)
		close(GLOBAL_PIPE[READ_END]);
		
	if(GLOBAL_PIPE2[WRITE_END] != 0)
		close(GLOBAL_PIPE2[WRITE_END]);
		
	if(GLOBAL_PIPE2[READ_END] != 0)
		close(GLOBAL_PIPE2[READ_END]);
}
/*************************************** 
 * Funções e Procedimentos Auxiliares
 ***************************************
 ***************************************/
void read_line(char* buffer){
	int c;  // variavel auxiliar (int pois EOF = -1)
	int pos = 0;  // indice da linha
	while(TRUE){
		c = getchar();
		if( c == EOF || c == '\n'){ // quebra de linha ou fim arqv
			buffer[pos] = '\0';// indica fim da linha
			return;  // leitura completa
		}else{
			buffer[pos] = c;  // append no buffer de linha
		}
		pos++;  // anda com o 'cursor'
	}
}
void print_commands(char** matrix){
	int i =0;
	printf("\033[1;35m---------------\n Comandos:\n");
	while(matrix[i] != NULL){
		if(is_operador(matrix[i]))
			printf("	Operador %d: (%s)\n", i, matrix[i]);
		else if (is_command(matrix[i]))
			printf("	Comando %d: <<%s>>\n", i, matrix[i]);
		/*else
			print_error("	Operador/Comando invalido");*/
		i++;
	}
	printf("---------------\033[0m\n");
}
int is_operador_char(char c){
	if( c  == '|' || c == '&' || c == '<' || c == '>')
		return TRUE;
	return FALSE;
}
int is_operador(char* c){
	if(c == NULL)  // importante
		return FALSE;
	int i = 0;
	int cursor = c[i];
	while(cursor != '\0' && cursor != EOF)
		cursor = c[++i];
	if(i != 1)  // operador de tamanho > 1
		return FALSE;
	// Verifica no primeiro(e unico) char
	return is_operador_char(c[0]);
}
int is_command(char* c){
	if(c == NULL)
		return FALSE;
	// verifica se tem um operador dentro do comando
	int i = 0;
	int cursor = c[i];
	while(cursor != '\0' && cursor != EOF){
		if(is_operador_char(cursor))
			return FALSE;
		cursor = c[++i];
	} 
	return TRUE;
}
void print_error(char* c){
	printf("\033[1;31mErro: %s\033[0m\n", c);
}
void print_alert(char* c){
	printf("\033[01;33m--> %s \033[0m\n", c);
}
void printf_alert(char* format, char* c){
	printf("\033[01;33m--> ");
	printf(format, c);
	printf("\033[0m\n");
}
