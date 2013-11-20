#include "link-includes.h"
#include <string.h>
#define THRESH 20
#define MAX 100

Dictionary dict;
Parse_Options opts;
Sentence sent;
Linkage linkage;
CNode * cn;
char * string;

int linklabel_counter = 0;
int graph[MAX][MAX];

int word_count = 0;

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

int is_same_word(char* a, char* b)
{
	int i = 0;
	char c[1024];
	char d[1024];

	for (i = 0; a[i]; i++)
	{
		c[i] = tolower(a[i]);
	}
	for (i = 0; b[i]; i++)
	{
		d[i] = tolower(b[i]);
	}

	if (strcmp(c, d) == 0)
		return 1;
	return 0;
}
//-----------------------------------------------------------------------------
typedef struct word_block_
{
	char* link;
	char* left_word;
	char* right_word;

} word_block;

void initialize(word_block* wb, char* link, char* left_word, char* right_word)
{
	wb->left_word = left_word;
	wb->right_word = right_word;
	wb->link = link;
}

//-----------------------------------------------------------------------------

struct hashMap
{
	char* linkLabel;
	int value;

} linklabel_map[MAX];

//-- Is the link already present in the hashmap
int isPresent(char* link)
{
	int i;
	for (i = 0; i < linklabel_counter; i++)
	{
		if (strcmp(link, linklabel_map[i].linkLabel) == 0)
			return 1;
	}

	return 0;
}

// -1 => invalid
int getLinkValue(char* link)
{
	int i;
	for (i = 0; i < linklabel_counter; i++)
	{
		if (strcmp(link, linklabel_map[i].linkLabel) == 0)
			return linklabel_map[i].value;
	}
	return -1;
}

char* getLinkLabelFromValue(int value)
{
	int i;
	for (i = 0; i < linklabel_counter; i++)
	{
		if (linklabel_map[i].value == value)
			return linklabel_map[i].linkLabel;
	}
	return NULL;

}

//-- Call this to initialize the graph
void initialize_graph()
{
	int i, j;
	for (i = 0; i < MAX; i++)
	{
		for (j = 0; j < MAX; j++)
		{
			graph[i][j] = -1;
		}
	}
}
void print_graph()
{
	int i, j;
	for (i = 0; i < word_count; i++)
	{
		for (j = 0; j < word_count; j++)
		{
			//-- This is true for the diagonal elements for sure, Maybe for others too
			if (graph[i][j] == -1)
			{
				printf("%s;%s;%s ", "NULL", linkage_get_word(linkage, i),
						linkage_get_word(linkage, j));
				continue;

			}
			printf("%s;%s;%s ", getLinkLabelFromValue(graph[i][j]),
					linkage_get_word(linkage, i), linkage_get_word(linkage, j));
		}
		printf("\n");
	}
}

int main()
{

	opts = parse_options_create();
	dict = dictionary_create("4.0.dict", "4.0.knowledge",
			"4.0.constituent-knowledge", "4.0.affix");

	initialize_graph();
//while (1)
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
				if (ner == 1)
				{
					entity_types[word_count][entity_char_count++] = c;
				} else
				{
					words[word_count][word_char_count++] = c;
					input_string[char_count++] = c;
				}

			}
		}

		//printf ( "%s\n%s\n", words[3],entity_types[3] );
		char* c1 = "This";
		char* c2 = "this";
		printf("%s\n", input_string);
		printf("%d\n", is_same_word(c1, c2));
		fflush(stdout);

		sent = sentence_create(input_string, dict);
		if (sentence_parse(sent, opts))
		{
			linkage = linkage_create(0, sent, opts);
			//-- Just setting this for usage later.
			word_count = linkage_get_num_words(linkage);

			printf("%s\n", string = linkage_print_diagram(linkage));
			string_delete(string);
			cn = linkage_constituent_tree(linkage);
			print_words_with_prep_phrases_marked(cn);
			linkage_free_constituent_tree(cn);
			//printf("All links \n%s\n", linkage_print_links_and_domains(linkage));

			int link_count = linkage_get_num_links(linkage);
			printf("%d\n", link_count);
			int i;
			//-- Array of structures of word blocks
			//word_block wb[]

			for (i = 0; i < link_count; i++)
			{
				char* link = linkage_get_link_label(linkage, i);
				char* left_word = linkage_get_word(linkage,
						linkage_get_link_lword(linkage, i));
				char* right_word = linkage_get_word(linkage,
						linkage_get_link_rword(linkage, i));

				//-- We now assign a unique int to every link.
				//-- Add it to the hashmap only if the link is not already present.
				if (!isPresent(link))
				{
					linklabel_map[linklabel_counter].linkLabel = (char*) malloc(
					MAX * sizeof(char));
					strcpy(linklabel_map[linklabel_counter].linkLabel, link);
					linklabel_map[linklabel_counter].value = linklabel_counter;
					linklabel_counter++;

				}
				//-- the get_link_rword returns an integer which corresponds to the word being returned.
				//-- This number is the index of the word and is small
				//-- We will now add the corresponding link between the l and r words to our graph using:

				graph[linkage_get_link_lword(linkage, i)][linkage_get_link_rword(
						linkage, i)] = getLinkValue(link);

				printf("%s;%s;%s\n", link, left_word, right_word);//, left_word, right_word);
			}
			print_graph();
		} else
		{
			//printf ( "Unable to parse sentence\n" );
		}
		sentence_delete(sent);

	}

	dictionary_delete(dict);
	parse_options_delete(opts);
	return 0;
}
