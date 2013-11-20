#include "link-includes.h"
#include <string.h>
#define THRESH 20

/* Print out the words at the leaves of the tree,
 bracketing constituents labeled "PP" */

void print_words_with_prep_phrases_marked(CNode *n)
{
	CNode * m;
	static char * spacer = " ";

	if (n == NULL)
		return;
	if (strcmp(n->label, "PP") == 0)
	{
		printf("%s[", spacer);
		spacer = "";
	}
	for (m = n->child; m != NULL; m = m->next)
	{
		if (m->child == NULL)
		{
			printf("%s%s", spacer, m->label);
			spacer = " ";
		} else
		{
			print_words_with_prep_phrases_marked(m);
		}
	}
	if (strcmp(n->label, "PP") == 0)
	{
		printf("]");
	}
}

int is_end_of_sentence(char c)
{
	if (c == '.' || c == '?' || c == '!' || c == ';')
		return 1;
	return 0;
}

int main()
{
	Dictionary dict;
	Parse_Options opts;
	Sentence sent;
	Linkage linkage;
	CNode * cn;
	char * string;

	opts = parse_options_create();
	dict = dictionary_create("4.0.dict", "4.0.knowledge",
			"4.0.constituent-knowledge", "4.0.affix");
	int high = 0;
	int overall = 0;
	while (1)
	{
		//-- This is the input string
		char input_string[1024 * 8];

		//-- Each row is a word. There can be atmost 100 words here.
		//-- The entities corresponding to the words in the above string.
		char words[100][1024];
		char entity_types[100][1024];

		//-- We now read the input string character by character until the end of the sentence
		int char_count = 0;
		//-- Boolean to read ner
		int word_count = 0;
		int word_char_count = 0;
		int entity_char_count = 0;
		int ner = 0;
		while (1)
		{
			char c;
			scanf("%c", &c);
			if (is_end_of_sentence(c))
			{
				input_string[char_count++] = c;
				entity_types[word_count][entity_char_count++] = c;
				input_string[char_count] = '\0';
				break;
			} else
			{
				if (c == '/')
				{
					ner = 1;
					entity_char_count = 0;
					continue;
				}
				if (c == ' ')
				{
					input_string[char_count++] = c;
					word_char_count = 0;
					ner = 0;
					word_count++;
					continue;
				}
				if ( ner == 1 )
				{
					entity_types[word_count][entity_char_count++] = c;
				}
				else
				{
					words[word_count][word_char_count++] = c;
				}
				input_string[char_count++] = c;
			}
		}

		printf ( "%s\n%s\n", words[3],entity_types[3] );
		fflush (stdout);

		sent = sentence_create(input_string, dict);
		if (sentence_parse(sent, opts))
		{
			linkage = linkage_create(0, sent, opts);
			printf("%s\n", string = linkage_print_diagram(linkage));
			string_delete(string);
			cn = linkage_constituent_tree(linkage);
			print_words_with_prep_phrases_marked(cn);
			linkage_free_constituent_tree(cn);

//		int i = 0;
//		for ( i = 0; i < linkage_get_num_links(linkage); i++ )
//		{
//			int link_label = linkage_get_link_label(linkage, i);
//			printf ( "%d \n", link_label );
//		}
			int num = sentence_num_linkages_found(sent);
			if (num > THRESH)
			{
				printf("\nNumber of Linkages found = %d\n", num);
				high++;
			}
			overall++;
			fprintf(stdout, "\n\n");
			linkage_delete(linkage);
		} else
		{
			//printf ( "Unable to parse sentence\n" );
		}
		sentence_delete(sent);

	}
	printf("%d %d\n", high, overall);

	dictionary_delete(dict);
	parse_options_delete(opts);
	return 0;
}
