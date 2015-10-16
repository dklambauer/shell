#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "shell.h"

/* Determine if a token is a special operator (like '|') */
int is_operator(char *token) {
	/** 
	 * Optional: edit this if you wish to parse other operators
	 * like ";", "&&", etc.
	 */
	return (strcmp(token, "|") == 0);
}

/* Determine if a command is builtin */
int is_builtin(char *token) {
	/** 
	 * Optional: Edit this if you wish to handle more builtin commands 
	 */
	if (strcmp(token, "cd") == 0) {
		return BUILTIN_CD;
	}
	if (strcmp(token, "exit") == 0) {
		return BUILTIN_EXIT;
	}
	return 0;
}

/* Determine if a path is relative or absolute (relative to root) */
int is_relative(char* path) {
	return (path[0] != '/'); 
}

/* Determine if a command is complex (has an operator like pipe '|') */
int is_complex_command(char **tokens) {
	
	int i = 0;
	while(tokens[i]) {		
		if (is_operator(tokens[i]))
			return 1;
		i++;
	}	
	return 0;
}

/* Parse a line into its tokens/words */
void parse_line(char *line, char **tokens) {
	
	while (*line != '\0') {
		/* Replace all whitespaces with \0 */
		while (*line == ' ' || *line == '\t' || *line == '\n') { 
			*line++ = '\0';
		}

		if (*line == '\0') {
      			break;
		}
			
		/* Store the position of the token in the line */
		*tokens++ = line;
		//printf("token: %s\n", *(tokens-1));
		
		/* Ignore non-whitespace, until next whitespace delimiter */
		while (*line != '\0' && *line != ' ' && 
		       *line != '\t' && *line != '\n')  {
			line++;                 
		}
	}
	*tokens = '\0';
}

int extract_redirections(char** tokens, simple_command* cmd) {
	
	int i = 0;
	int skipcnt = 0;
	
	while(tokens[i]) {
		
		int skip = 0;		
		if (!strcmp(tokens[i], ">")) {
			if(!tokens[i+1]) {
				return -1;
			}
			cmd->out = tokens[i+1];
			skipcnt += 2;
			skip = 1;
		}			
		if (!strcmp(tokens[i], "<")) {
			if(!tokens[i+1]) {
				return -1;
			}
			cmd->in = tokens[i+1];
			skipcnt += 2;
			skip = 1;
		}			
		if (!strcmp(tokens[i], "2>")) {
			if(!tokens[i+1]) {
				return -1;
			}
			cmd->err = tokens[i+1];
			skipcnt += 2;
			skip = 1;
		}
		if (!strcmp(tokens[i], "&>")) {
			if(!tokens[i+1]) {
				return -1;
			}
			cmd->out = tokens[i+1];
			cmd->err = tokens[i+1];
			skipcnt += 2;
			skip = 1;
		}
			
		if(skip){   
			i++;
		}
		
		i++;
	}
	
	cmd->tokens = malloc((i-skipcnt+1) * sizeof(char*));	
	
	int j = 0;
	i = 0;
	while(tokens[i]) {
		if (!strcmp(tokens[i], "<") ||
		    !strcmp(tokens[i], ">") ||
		    !strcmp(tokens[i], "2>") ||
		    !strcmp(tokens[i], "&>")) {
			i += 2;
		 } else {
			cmd->tokens[j++] = tokens[i++];
		 }
	}
	cmd->tokens[j] = NULL;
	
	return 0;
}

/* Construct command */
command* construct_command(char** tokens) {

	/* Initialize a new command */	
	command *cmd = malloc(sizeof(command));
	cmd->cmd1 = NULL;
	cmd->cmd2 = NULL;
	cmd->scmd = NULL;

	if (!is_complex_command(tokens)) {
		
		/* Simple command */
		cmd->scmd = malloc(sizeof(simple_command));
		cmd->scmd->in = NULL;
		cmd->scmd->out = NULL;
		cmd->scmd->err = NULL;
		cmd->scmd->tokens = NULL;
		
		cmd->scmd->builtin = is_builtin(tokens[0]);
		
		int err = extract_redirections(tokens, cmd->scmd);
		if (err == -1) {
			printf("Error extracting redirections!\n");	
			return NULL;
		}
	}
	else {
		/* Complex command */
		
		char **t1 = tokens, **t2;
		int i = 0;
		while(tokens[i]) {
			if(is_operator(tokens[i])) {
				strncpy(cmd->oper, tokens[i], 2);
				tokens[i] = NULL;
				t2 = &(tokens[i+1]);
				break;
			}
			i++;
		}
		
		/* Recursively construct the rest of the commands */
		cmd->cmd1 = construct_command(t1);
		cmd->cmd2 = construct_command(t2);
	}
	
	return cmd;
}

/* Release resources */
void release_command(command *cmd) {
	
	if(cmd->scmd && cmd->scmd->tokens) {
		free(cmd->scmd->tokens);
	}
	if(cmd->cmd1) {
		release_command(cmd->cmd1);
	}
	if(cmd->cmd2) {
		release_command(cmd->cmd2);		
	}
}

/* Print command */
void print_command(command *cmd, int level) {
	
	int i;
	for(i = 0; i < level; i++) {
		printf("  ");
	}
	
	if(cmd->scmd) {
		
		i = 0;
		while(cmd->scmd->tokens[i]) { 
			printf("%s ", cmd->scmd->tokens[i]);
			i++;
		}
		
		if(cmd->scmd->in) {
			printf("< %s ", cmd->scmd->in);
		}

		if(cmd->scmd->out) {
			printf("> %s ", cmd->scmd->out);
		}

		if(cmd->scmd->err) {
			printf("2> %s ", cmd->scmd->err);
		}
			
		printf("\n");
		return;		 
	}
	
	printf("Pipeline:\n");
			
	if(cmd->cmd1) {
		print_command(cmd->cmd1, level+1);
	}

	if(cmd->cmd2) {
		print_command(cmd->cmd2, level+1);
	}
	
}
