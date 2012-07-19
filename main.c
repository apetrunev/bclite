#include <stdio.h>
#include <stdlib.h>

#include "syntax.h"
#include "traverse.h"
#include "as_tree.h"
#include "symbol.h"
#include "keyword.h"
#include "function.h"
#include "macros.h"

static char *prompt; /* `> ' or nothing */
static FILE *input;  /* if no file is specified we read from stdin */

static void
print_info(void)
{
	printf( "\n"
		"\tVersion 1.0\n"
		"\tCopyleft 2012\n"
		"\tSyktyvkar State University\n"
	        "\tProgramming & Applied Math Laboratory\n"
		"\n");	
}

static void
parse_args(int argc, char **argv)
{
	if (argc < 2) {
		input = stdin;
		set_file(input);
		print_info();
		prompt = "> ";
	} else {
		input = fopen(argv[1], "r");
		if (input == NULL) {
			fprintf(stderr, "error: cannot open the file %s\n", argv[1]);
			return;
		}
		set_file(input);
		prompt = "";
	}	
}

static void
get_result(struct ast_node *tree, int errors)
{
	return_if_fail(tree != NULL);

	if (!errors) {
		traversal(tree);
		traversal_print_result();
	}
	
	ast_node_unref(tree);
}

static void
close_stream(FILE *input)
{
	return_if_fail(input != NULL);

	if (input != stdin)
		fclose(input);
}

static void
gsl_handler(const char *reason, const char *file, int line, int gsl_errno)
{
	switch(gsl_errno) {
	case GSL_ENOMEM:
		fprintf(stderr, "Reason: %s File: %s line: %d",
			reason, file, line);		
		exit(1);
		break;
	default:
		fprintf(stderr, "Reason: %s File: %s line: %d",
			reason, file, line);
		break;
	}
}

int
main(int argc, char **argv)
{
	struct ast_node *tree;
	int eof, errors;
							
	symbol_table_create_global();
	keyword_table_create();
	function_table_create();
	
	/* set our handler*/
	gsl_set_error_handler(gsl_handler);
	parse_args(argc, argv);

	do {	
		fputs(prompt, stdout);

		errors = programme(&tree, &eof);
		
		get_result(tree, errors);
				
	} while (!eof);
	
	close_stream(input);
	
	return 0;
}
	
