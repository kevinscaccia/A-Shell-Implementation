/* Copyright (C) Kevin Costa Scaccia - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Written by Kevin Scaccia <kevin_developer@outlook.com>, March 2019
 */

/************************
	Shell functionalities
*******************
1. Single Commands(without args):
	>> ls
	>> ps 
	>> clear
*******************
2. Single Commands(with args):
	>> ls -ax
	>> ps all	
*******************
3. Commands(without args) related by a unique PIPE:
	>> ps | grep b
	>> ls | grep out
*******************
4. Commands(with args) related by a unique PIPE:
	>> ps all | grep gnome
	>> ls -ax | grep out
*******************
5. Commands(with args) related by any PIPE:
	>> ps all | grep gnome
	>> ls -ax | grep out
	>> ls -ax | grep a | grep b | grep a | more
	>> ls | ls | ls | ps | ps -ax | echo test | grep test
*******************
6. One or Two Commands(with args) to run assync:
	>> gedit file.txt & tail file.txt -F 
	>> ps all & gedit
	>> ping www.google.com & ls
	>> gedit & ls
*******************
6. Multiple Commands(with args) to run assync:
	>> ps & ps -ax & gedit
	>> ls & ls & ls & nano
*******************
7. Multiple '&' operator followed by multiple '|':
	>> gedit file.txt & gedit file2.txt & ps all | grep gnome | grep keyboard
*******************
8. Redirect '>' operator, for one command or from a pipe to a file
	>> ls > file.txt
	>> echo "kevin" > file.txt
        >> ps all | grep a > file.txt
*******************
9. K-Shell internal Commands:
	>> close: close the shell
	>> help: show internal commands


*/
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
#include <fcntl.h> 
/*************************************** 
 * Constants
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
/*************************************** 
 * Prototypes
 ***************************************
 ***************************************/
// Shell LifeCycle Functions
void start_shell();
void loop_shell();
void finish_shell();
// Command Manipulation
char** split_commands(char* line);
char** get_command_parameters(char** matrix, int* index);
// Command Execution
int execute_commands(char* line);
int execute_standard_sync_command(char** argv);
int execute_standard_async_command(char** argv);
// Operator Position Idenftify
int cmd_no_operator(char** argv);  // command without operator
int cmd_before_operator(char** argv, char* operator, char** cmd_tokens, int* index);
int cmd_between_operator(char** argv, char* before_operator, char* after_operator, char** cmd_tokens, int* index); // command between operators
int cmd_after_operator(char** argv, char* operator);  // command after operator
int is_operator(char* str, char operator);
int exists_operator_before(char** commands, int index);  // if there's a operator before the index
int exists_operator_after(char** commands, int index);  // if there's a operator after the index
// Operator Handling
int cmd_before_operator_handle_pipe(char** argv);  // command before operator
int cmd_between_operator_handle_pipe_pipe(char** argv);  // command between pipes
int cmd_between_operator_handle_redirect_out_pipe(char** argv, char** cmd_tokens, int* index); // command betweeen '|' and '>'
int cmd_after_operator_handle_pipe(char** argv);  // command after operator
int cmd_before_operator_handle_background(char** commands, int* index);  // command to run in background
int cmd_before_operator_handle_redirect_out(char** argv, char** cmd_tokens, int* index);

// Internal Commands
int execute_internal_help();
int execute_internal_last();
// Auxiliary Functions and Procedures
int count_args(char** argv, int index);
void close_fd(int fd);
void read_line(char* buffer);
int is_operador(char* c);
int is_operador_char(char c);
int is_command(char* c);
void close_pipes();
// Print and Format Procedures
void print_commands(char** command_matrix);
void print_error(char* c);
void print_alert(char* c);
void printf_alert(char* format, char* c);
/*************************************** 
 * Global Variables
 ***************************************
 ***************************************/
int GLOBAL_PIPE1[2];
int GLOBAL_PIPE2[2];
int MULTIPLE_PIPES_FLAG = FALSE;
char LAST_COMMAND_LINE[MAX_COMMAND_SIZE];
char* LAST_COMMAND_MATRIX;
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
 * Shell LifeCycle
 ***************************************
 ***************************************/
void start_shell(){
	printf("\033[0;1m"); // white bold
	printf("---------------\033[1;32m K-Shell\033[0;1m ---------------");
	printf("\n---------------------------------------\n");
}
void loop_shell(){
	char *line;  // line read buffer
	int status; // commands return status
	do{
		line = (char*) malloc(sizeof(char)*LINE_BUFFER_SIZE);
		printf("\033[0;1m >> \033[0m");  
		read_line(line);// reads the line from stdin
		// parse commands in a command matrix
		status = execute_commands(line);
		free(line);// I don't know if it's necessary
	}while(status == SHELL_STATUS_CONTINUE);

}
void finish_shell(){
	close_pipes();
	printf("\033[0;1m----------------------------------\033[1;31mbye\033[0;1m..\n");
}
/*************************************** 
 * Command Management
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
/*************************************** 
 * Execução dos Comandos
 ***************************************
 ***************************************/
int execute_commands(char* line){
	char** cmd_tokens = split_commands(line);  // Split line in command tokens
	//print_commands(cmd_tokens);
	int i = 0, status;
	while(cmd_tokens[i] != NULL){
		if(is_command(cmd_tokens[i])){// encontrou um COMANDO
			char** argv; // Command Args
			if(exists_operator_before(cmd_tokens, i)){
				if(exists_operator_after(cmd_tokens, i)){
					print_alert("There's a command between operators");
					char* before_operator = cmd_tokens[i-1];  // before operator (always is the token before the command itself)
					argv = get_command_parameters(cmd_tokens, &i);// get command arguments
					char* after_operator = cmd_tokens[i];  // after operator
					status = cmd_between_operator(argv, before_operator, after_operator, cmd_tokens, &i);// handle operator and execute the command
				}else{
					print_alert("There's a operator before command");
					char* operator = cmd_tokens[i-1];  // actual operator (always is the token before the command itself)
					argv = get_command_parameters(cmd_tokens, &i);// get command arguments
					status = cmd_after_operator(argv, operator);// handle operator and execute the command
				}
			}else if(exists_operator_after(cmd_tokens, i)){
				print_alert("There's a operator after command");
				argv = get_command_parameters(cmd_tokens, &i);// get command arguments
				char* operator = cmd_tokens[i];  // actual operator
				status = cmd_before_operator(argv, operator, cmd_tokens, &i);// handle operator and execute the command
			}else{
				print_alert("Single Command");
				argv = get_command_parameters(cmd_tokens, &i);// get command arguments
				status = cmd_no_operator(argv);
			}
		}
		i++; // Next Token
	}
	close_pipes();
	MULTIPLE_PIPES_FLAG = FALSE;
	strcpy(LAST_COMMAND_LINE, line); // Store last command line
	printf("\033[0m");
	return status;// SHELL_STATUS_CONTINUE
}
int execute_standard_sync_command(char** argv){
	pid_t child;
	//print_commands(com_mat);
	child = fork();
	if(child == 0){
		if(execvp(argv[0], argv) < 0){
			print_error("Comando possívelmente invalido ou incompleto:");
			perror("->");
		}
	}else{
		waitpid(child, NULL, 0);
	}
	return SHELL_STATUS_CONTINUE;
}
int execute_standard_async_command(char** argv){
	print_alert("Background Command");
	pid_t child;
	child = fork();// FORK NO child
	if(child == 0){  // PRIMEIRO COMANDO
		if(execvp(argv[0], argv) < 0){  // EXECUTA O COMANDO
			print_error("Comando possívelmente invalido ou incompleto:");
			perror("->");
		}
	}else{ // CODIGO DO PAI
		//waitpid(child, NULL, 0); Aqui esta o segredo hehe
		print_alert("Async command finished");
	}
	return SHELL_STATUS_CONTINUE;
}
/*************************************** 
 * Operator Position Idenftify
 ***************************************
 ***************************************/
int cmd_no_operator(char** argv){
	// Comandos internos do shell
	if( strcmp(argv[0], "close") == 0){
		return SHELL_STATUS_CLOSE;
	}else if( strcmp(argv[0], "help") == 0){
		return execute_internal_help();
	}else if( strcmp(argv[0], "last") == 0){
		return execute_internal_last();
	}else  // Single Command
		return execute_standard_sync_command(argv);	
}
int cmd_before_operator(char** argv, char* operator, char** cmd_tokens, int* index){
	if( is_operator(operator, '|') ){  // handles pipe operator
		return cmd_before_operator_handle_pipe(argv);
	}else if( is_operator(operator, '&') ){  // handles background command
		return execute_standard_async_command(argv);
	}else if( is_operator(operator, '>') ){  // handles redirect_out operator
		return cmd_before_operator_handle_redirect_out(argv, cmd_tokens, index);
	}else{
		print_error("Operador nao identificado");
		return SHELL_STATUS_CONTINUE;
	}
}
int cmd_between_operator(char** argv, char* before_operator, char* after_operator, char** cmd_tokens, int* index){
	// Handles Command between two PIPEs 
	if( is_operator(before_operator, '|') && is_operator(after_operator, '|')){
	        print_alert("| e |");
		return cmd_between_operator_handle_pipe_pipe(argv);
	}else if( is_operator(before_operator, '&') &&  is_operator(after_operator, '&')){  // Assync command
		print_alert("& e &");
		return execute_standard_async_command(argv);  // Just run assync and go to the next command
	}else if( is_operator(before_operator, '&') &&  is_operator(after_operator, '|')){  // Assync command
		print_alert("& e |");
		return cmd_before_operator_handle_pipe(argv);  // Handle the pipe after
	}else if( is_operator(before_operator, '|') &&  is_operator(after_operator, '>')){  // pipe redirect
	        print_alert("| e >");
	        return cmd_between_operator_handle_redirect_out_pipe(argv, cmd_tokens, index);
	}
}
int cmd_after_operator(char** argv, char* operator){
	if( is_operator(operator, '|') ){  // handles pipe operator
		return cmd_after_operator_handle_pipe(argv);
	}else if( is_operator(operator, '&') ){  // handles background cmd
		return execute_standard_sync_command(argv);
	}
}
/*************************************** 
 * Operator Handling
 ***************************************
 ***************************************/
int cmd_before_operator_handle_pipe(char** argv){
	// PRIMEIRO SEMPRE ESCREVE NO PIPE 1
	MULTIPLE_PIPES_FLAG = !MULTIPLE_PIPES_FLAG;
	pid_t child;
	pipe(GLOBAL_PIPE1);// CRIA PIPE 1
	child = fork();// FORK NO child
	if(child == 0){  // PRIMEIRO COMANDO
		close_fd(GLOBAL_PIPE1[READ_END]);// child NUNCA LÊ DO PIPE 1
		dup2(GLOBAL_PIPE1[WRITE_END], STDOUT_FILENO); // child SEMPRE ESCREVE NO PIPE 1
		if(execvp(argv[0], argv) < 0){  // EXECUTA O COMANDO
			print_error("Comando possívelmente invalido ou incompleto:");
			return SHELL_STATUS_CONTINUE;
		}
	}else{ // CODIGO DO PAI
    	close_fd(GLOBAL_PIPE1[WRITE_END]);// PAI NAO ESCREVE NO PIPE 1 MAS PRECISA LER DELE (child)
		waitpid(child, NULL, 0);  // ESPERA child 1
		print_alert("First command finished");
	}
	return SHELL_STATUS_CONTINUE;
}
int cmd_between_operator_handle_pipe_pipe(char** argv){
	MULTIPLE_PIPES_FLAG = !MULTIPLE_PIPES_FLAG;
	pid_t child;
	if(MULTIPLE_PIPES_FLAG){ // 
		// CRIA PIPE 1
    	if(pipe(GLOBAL_PIPE1) < 0)
    		print_error("ERRO PIPE 1");
    	print_alert("criou pipe 1");
    }else{// PAR
    	// CRIA PIPE 2
    	if(pipe(GLOBAL_PIPE2) < 0)
    		print_error("ERRO PIPE 2");
    	print_alert("criou pipe 2");
    }
	child = fork();
	if(child == 0){
    	print_alert("Middle Command");
		if(MULTIPLE_PIPES_FLAG){ // IMPAR
			dup2(GLOBAL_PIPE2[READ_END], STDIN_FILENO);// LER DO PIPE 2
			dup2(GLOBAL_PIPE1[WRITE_END], STDOUT_FILENO); // ESCREVER NO PIPE 1
		}else{ // PAR
			dup2(GLOBAL_PIPE1[READ_END], STDIN_FILENO);// LER DO PIPE 1
			dup2(GLOBAL_PIPE2[WRITE_END], STDOUT_FILENO); // ESCREVER NO PIPE 2
		}
		if(execvp(argv[0], argv) < 0)
			print_error("Comando possívelmente invalido ou incompleto:");
	}else{
		if(MULTIPLE_PIPES_FLAG){ // IMPAR
			close_fd(GLOBAL_PIPE2[READ_END]);// JA PASSOU PARA O child
			close_fd(GLOBAL_PIPE1[WRITE_END]); // SOMENTE child ESCREVE NO PIPE
		}else{ 
			close_fd(GLOBAL_PIPE1[READ_END]);// JA PASSOU PARA O child
			close_fd(GLOBAL_PIPE2[WRITE_END]); // SOMENTE child ESCREVE NO PIPE
		}
		waitpid(child, NULL, 0);
		print_alert("child do meio terminou");
	}
	return SHELL_STATUS_CONTINUE;
}
int cmd_after_operator_handle_pipe(char** argv){
// ULTIMO COMANDO: SEMPRE ESCREVE NA SAIDA PADRÃO; PODE LER DO PIPE 1 OU DO 2
// LE DO PIPE 1 CASO NAO HOUVER COMANDOS INTERNOS OU TIVER UM NUMERO PAR DE COMANDOS INTERNOS
// LE DO PIPE 2 CASO HOUVER UM NUMERO IMPAR DE COMANDOS INTERNOS
	pid_t child;
	child = fork();
	if(child == 0){ 
		if(MULTIPLE_PIPES_FLAG){// IMPAR
			dup2(GLOBAL_PIPE1[READ_END], STDIN_FILENO); // LE DO PIPE 1
		}else{// OU LE DO 1 OU LE DO 2:
			dup2(GLOBAL_PIPE2[READ_END], STDIN_FILENO); // LE DO PIPE 2
		}
		if(execvp(argv[0], argv) < 0){
			print_error("Comando possívelmente invalido ou incompleto:");
			return SHELL_STATUS_CONTINUE;
		}
	}else{ // PAI
		if(MULTIPLE_PIPES_FLAG){// IMPAR
			close_fd(GLOBAL_PIPE1[READ_END]); // LE DO PIPE 1
		}else{// OU LE DO 1 OU LE DO 2:
			close_fd(GLOBAL_PIPE2[READ_END]); // LE DO PIPE 2
		}
		waitpid(child, NULL, 0);
		print_alert("Last command finished");
	}															
	return SHELL_STATUS_CONTINUE;
}
int cmd_before_operator_handle_redirect_out(char** argv, char** cmd_tokens, int* index){
	pid_t child;
	int args_after_cmd = count_args(cmd_tokens, *index + 1);// conta numero de 'argumentos' do proximo comando
	if( args_after_cmd != 1){ // um unico arquivo
		print_error("Especifique um unico arquivo: <cmd> > <arquivo.txt>");
		return SHELL_STATUS_CONTINUE;
	}
	char* file_name = cmd_tokens[*index + 1]; // Get file name
	print_alert("File name:"); print_alert(file_name);
	
	child = fork();// FORK
        if(child == 0){
                int fd = open(file_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
                dup2(fd, STDOUT_FILENO);   // make stdout go to file
                close(fd);
                if(execvp(argv[0], argv) < 0){  // EXECUTA O COMANDO
	                print_error("Comando possivelmente invalido ou incompleto:");
	                return SHELL_STATUS_CONTINUE;
	        }
        }else{ // CODIGO DO PAI
                waitpid(child, NULL, 0);  // ESPERA child 1
        }
	*index += 1; // proximo comando
	return SHELL_STATUS_CONTINUE;
}
int cmd_between_operator_handle_redirect_out_pipe(char** argv, char** cmd_tokens, int* index){
        print_alert("identificado");
        MULTIPLE_PIPES_FLAG = !MULTIPLE_PIPES_FLAG;
	pid_t child;
	if(MULTIPLE_PIPES_FLAG){ // 
    	        if(pipe(GLOBAL_PIPE1) < 0)
    		        print_error("ERRO PIPE 1");
    	        print_alert("criou pipe 1");
        }else{
                if(pipe(GLOBAL_PIPE2) < 0)
	                print_error("ERRO PIPE 2");
                print_alert("criou pipe 2");
        }
	int args_after_cmd = count_args(cmd_tokens, *index + 1);// conta numero de 'argumentos' do proximo comando
	if( args_after_cmd != 1){ // um unico arquivo
		print_error("Especifique um unico arquivo: <cmd> > <arquivo.txt>");
		return SHELL_STATUS_CONTINUE;
	}
	char* file_name = cmd_tokens[*index + 1]; // Get file name
	print_alert("File name:"); print_alert(file_name);
	child = fork();
	if(child == 0){
    	        print_alert("Middle Command");
		if(MULTIPLE_PIPES_FLAG){ // IMPAR
			dup2(GLOBAL_PIPE2[READ_END], STDIN_FILENO);// LER DO PIPE 2
		}else{ // PAR
			dup2(GLOBAL_PIPE1[READ_END], STDIN_FILENO);// LER DO PIPE 1
		}
		// ESCREVER NO ARQUIVO
		int fd = open(file_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
		dup2(fd, STDOUT_FILENO);   
		//
		close(fd);
		if(execvp(argv[0], argv) < 0)
			print_error("Comando possívelmente invalido ou incompleto:");
	}else{
		if(MULTIPLE_PIPES_FLAG){ // IMPAR
			close_fd(GLOBAL_PIPE2[READ_END]);// JA PASSOU PARA O child
			close_fd(GLOBAL_PIPE1[WRITE_END]); // SOMENTE child ESCREVE NO PIPE
		}else{ 
			close_fd(GLOBAL_PIPE1[READ_END]);// JA PASSOU PARA O child
			close_fd(GLOBAL_PIPE2[WRITE_END]); // SOMENTE child ESCREVE NO PIPE
		}
		waitpid(child, NULL, 0);
		print_alert("child do meio terminou");
	}
	*index += 1; // proximo comando
	return SHELL_STATUS_CONTINUE;
}
/*************************************** 
 * Internal Commands
 ***************************************
 ***************************************/
int execute_internal_help(){
	print_alert("-- Shell Internal Commands: --");
	print_alert("	<help>: Show Internal Commands");
	print_alert("	<close>: Close K-Shell");
	return SHELL_STATUS_CONTINUE;
}
int execute_internal_last(){
	print_alert(LAST_COMMAND_LINE);
	return SHELL_STATUS_CONTINUE;
}
/*************************************** 
 * Funções e Procedimentos Auxiliares
 ***************************************
 ***************************************/
int count_args(char** argv, int index){
        int i = index;// Conta o numero de tokens a partir de 'index'
	int argc = 0;
	char* arg = argv[i];
	while(arg != NULL & !is_operador(arg)){
		arg = argv[++i];
		argc++;
	}
	return argc;
}
int exists_operator_before(char** commands, int index){
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
int exists_operator_after(char** commands, int index){
	char* cmd = commands[index];
	while(cmd != NULL){
		if(is_operador(cmd))
			return TRUE;// ha operador depois
		cmd = commands[++index];
	}
	return FALSE;
}
void close_pipes(){
	if(GLOBAL_PIPE1[WRITE_END] != 0)
		close(GLOBAL_PIPE1[WRITE_END]);
	if(GLOBAL_PIPE1[READ_END] != 0)
		close(GLOBAL_PIPE1[READ_END]);	
	if(GLOBAL_PIPE2[WRITE_END] != 0)
		close(GLOBAL_PIPE2[WRITE_END]);
	if(GLOBAL_PIPE2[READ_END] != 0)
		close(GLOBAL_PIPE2[READ_END]);
}
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
void close_fd(int fd){
	if(fd != 0 && fd != 1)
		close(fd);
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
int is_operator(char* c, char operator){
	if(c == NULL)  // important
		return FALSE;
	int i = 0;
	int cursor = c[i];
	while(cursor != '\0' && cursor != EOF)
		cursor = c[++i];
	if(i != 1)  // operador de tamanho > 1
		return FALSE;
	// Verifica no primeiro(e unico) char
	return (c[0] == operator);
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
