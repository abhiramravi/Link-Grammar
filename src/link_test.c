/*
 *
 *
 *
 *
 *
 *
 * The party begins in California/LOCATION at 5/TIME AM/TIME.
 * The party will happen in California/LOCATION at 5/TIME AM/TIME.
 * The party will be happening in California/LOCATION at 5/TIME AM/TIME.
 *
 * We will be conducting the party in California/LOCATION at 5/TIME AM/TIME.
 *
 *
 * */


#include "link-includes.h"
#include <string.h>
#include <stdlib.h>
#define THRESH 20
#define MAX 100

Dictionary dict;
Parse_Options opts;
Sentence sent;
Linkage linkage;
CNode * cn;
char * string;

//-- This is the input string
char input_string[1024 * 8];

//-- Each row is a word. There can be atmost 100 words here.
//-- The entities corresponding to the words in the above string.
char words[MAX][1024];
char entity_types[MAX][1024];

int linklabel_counter = 0;
int graph[MAX][MAX];
int transGraph[MAX][MAX];

int word_count = 0;
int entity_count = 0;
int num_relations = 0;

/* Builkd the transitive closure of the gfraph
 *
 */

void TransitiveClosure()
{
	int i, j, k;
	for (i = 0; i < MAX; i++)
	{
		for (j = 0; j < MAX; j++)
			transGraph[i][j] = graph[i][j];

	}

	for (k = 0; k < MAX; k++)
	{
		for (i = 0; i < MAX; i++)
		{
			for (j = 0; j < MAX; j++)
			{
				if (transGraph[i][j] == -1)
				{
					if (transGraph[i][k] != -1 && transGraph[k][j] != -1)
						transGraph[i][j] = 0; // JUst checking for connectivity
				}
			}

		}

	}

}

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

	for (i = 0; i < strlen(a); i++)
	{
		c[i] = tolower(a[i]);
	}
	c[i] = '\0';
	for (i = 0; i < strlen(b); i++)
	{
		d[i] = tolower(b[i]);
	}
	d[i] = '\0';

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
				printf("%s;%s;%s ", "NULL", linkage_get_word(linkage, i), linkage_get_word(linkage, j));
				continue;

			}
			printf("%s;%s;%s ", getLinkLabelFromValue(graph[i][j]), linkage_get_word(linkage, i),
					linkage_get_word(linkage, j));
		}
		printf("\n");
	}
}

//-- This functions given a word
//-- if NE is defined, return the NE from entity array
//-- else return O as default
char* get_named_entity(char* word_orig)
{
	//-- word is something like this.p. Need to strip

	char word[MAX];
	strcpy(word, word_orig);
	int i = strlen(word);
	while (word[i] != '.' && i >= 0)
		i--;
	if (i != 0)
		word[i] = '\0';

	//printf("II = %d\n",entity_count);
	for (i = 0; i < entity_count; i++)
	{
		//printf("WORDs = %s;%s\n",word,words[i]);
		if (is_same_word(words[i], word) == 1)
		{
			//printf("WENT HERE");
			return entity_types[i];
		}
	}
	return "O";
}

struct LocationTimeDependency
{
	char* location;
	char* time;
	char* subject;

} relations[MAX];

/*
 * These two functions take in an input word index, and finds if there
 * is a link of the type link_type from it. If so, it returns
 * the index of the linked word.
 *
 * */
int isThereALink(int word_index, char link_type)
{
	int k;
	for (k = 0; k < MAX; k++)
	{
		if (graph[word_index][k] == -1)
			continue;
		if (getLinkLabelFromValue(graph[word_index][k])[0] == link_type) //XXX: Ther might another link which starts with S
			break;
	}
	return k;
}

int main()
{

	opts = parse_options_create();
	dict = dictionary_create("4.0.dict", "4.0.knowledge", "4.0.constituent-knowledge", "4.0.affix");

	initialize_graph();
//while (1)
	{

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
				//entity_types[word_count][entity_char_count++] = c;
				entity_types[word_count][entity_char_count++] = '\0';
				input_string[char_count] = '\0';
				word_count++;
				break;
			} else
			{
				if (c == '/')
				{
					ner = 1;
					words[word_count][word_char_count++] = '\0';
					entity_char_count = 0;
					continue;
				}
				if (c == ' ')
				{
					entity_types[word_count][entity_char_count++] = '\0';
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
			entity_count = word_count;
			word_count = linkage_get_num_words(linkage);

			printf("%s\n", string = linkage_print_diagram(linkage));
			string_delete(string);
			cn = linkage_constituent_tree(linkage);
			print_words_with_prep_phrases_marked(cn);
			linkage_free_constituent_tree(cn);
			//printf("All links \n%s\n", linkage_print_links_and_domains(linkage));

			int link_count = linkage_get_num_links(linkage);
			//printf("%d\n", link_count);
			int i;
			//-- Array of structures of word blocks
			//word_block wb[]

			for (i = 0; i < link_count; i++)
			{
				char* link = linkage_get_link_label(linkage, i);
				char* left_word = linkage_get_word(linkage, linkage_get_link_lword(linkage, i));
				char* right_word = linkage_get_word(linkage, linkage_get_link_rword(linkage, i));

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

				graph[linkage_get_link_lword(linkage, i)][linkage_get_link_rword(linkage, i)] = getLinkValue(link);

				graph[linkage_get_link_rword(linkage, i)][linkage_get_link_lword(linkage, i)] = getLinkValue(link);
				//printf("%s;%s;%s\n", link, left_word, right_word);//, left_word, right_word);
			}

			//print_graph();

			//find the word which is NER tagged with location
			// We will simply need the subject form of the word tagged with location
			//To identify the subject acting on the location, find a word such that the location is connected to that word
			// and it has a right S link out of it.

			TransitiveClosure();

			for (i = 0; i < word_count; i++)
			{
				//printf ( "%s\n", get_named_entity(linkage_get_word(linkage, i)));
				if (strcmp("LOCATION", get_named_entity(linkage_get_word(linkage, i))) == 0)
				{
					int j;

					for (j = 0; j < MAX; j++)
					{
						int valid = 0;
						if (transGraph[i][j] != -1)
						{
							int k;
							for (k = 0; k < MAX; k++)
							{
								if (graph[j][k] == -1)
									continue;
								if (getLinkLabelFromValue(graph[j][k])[0] == 'S') //XXX: Ther might another link which starts with S
								{
									valid = 1;
									break;
								}
							}
							if (valid)
								break;
						}
					}
					// j - is the required subject
					//printf("%d\n",j);
					printf("\nSUBJECT = %s\n", linkage_get_word(linkage, j));
					int subject = j;

					int k;
					for (k = 0; k < MAX; k++)
					{
						if (graph[j][k] == -1)
							continue;
						if (getLinkLabelFromValue(graph[j][k])[0] == 'S') //XXX: Ther might another link which starts with S
							break;
					}
					// k - is the required other end of subject link
					printf("\nEND OF SUBJECT = %s\n", linkage_get_word(linkage, k));

					int endOfSubject = k;

					k = 0;

					int current_word = endOfSubject;
					int i_state = 0;
					int p_state = 0;

					while (k < MAX)
					{
						//-- Backing up the value of k for our state machine's optinal states
						int k_backup = k;

						//-- Check for an optional I state
						if (!i_state)
						{
							for (; k < MAX; k++)
							{
								if (graph[current_word][k] == -1)
									continue;
								if (getLinkLabelFromValue(graph[current_word][k])[0] == 'I') //XXX:
								{
									printf("State i found %s\n", linkage_get_word(linkage, k));
									current_word = k;
									i_state = 1;
									break;
								}
							}

							if (i_state)
							{
								for (; k < MAX; k++)
								{
									if (graph[current_word][k] == -1)
										continue;
									if (getLinkLabelFromValue(graph[current_word][k])[0] == 'P') //XXX:
									{
										printf("State p found %s\n", linkage_get_word(linkage, k));
										current_word = k;
										p_state = 1;
										continue;
									}
								}
							}

							if ( k >= MAX )
								k = k_backup;
						}
						for (; k < MAX; k++)
						{
							if (graph[current_word][k] == -1)
								continue;
							if (getLinkLabelFromValue(graph[current_word][k])[0] == 'M'
									&& getLinkLabelFromValue(graph[current_word][k])[1] == 'V') //XXX:

								break;
						}
						// k - is the required other end of subject link
						if (k >= MAX)
							break;

						printf("\nPreposition = %s\n", linkage_get_word(linkage, k));

						int preposition = k;

						int l;
						// Required action on seeing a preposition
						for (l = 0; l < MAX; l++)
						{
							if (graph[preposition][l] == -1)
								continue;
							if (getLinkLabelFromValue(graph[preposition][l])[0] == 'J') //XXX:
							{
								char* objectOfPreposition = linkage_get_word(linkage, l);
								printf("%s\n", objectOfPreposition);
								if (strcmp(get_named_entity(objectOfPreposition), "TIME") == 0)
								{

									int tempLoop = l - 1;
									while (tempLoop >= 0
											&& strcmp(get_named_entity(linkage_get_word(linkage, tempLoop)), "TIME")
													== 0)
									{
										tempLoop--;
									}
									char result[1024];
									result[0] = '\0';

									printf("Result = %s\n", result);
									tempLoop++;

									while (tempLoop < word_count
											&& strcmp(get_named_entity(linkage_get_word(linkage, tempLoop)), "TIME")
													== 0)
									{
										strcat(result, linkage_get_word(linkage, tempLoop));
										strcat(result, " ");
										tempLoop++;
									}
									/*printf("Corresponding Time word is %s\n",
									 result);*/

									relations[num_relations].subject = linkage_get_word(linkage, subject);
									relations[num_relations].location = linkage_get_word(linkage, i);
									relations[num_relations].time = result;
									num_relations++;
									break;
								}

							}
						}

						k++;
					}
				}

			} // End of outer loop

			for (i = 0; i < num_relations; i++)
			{
				printf("LT_Relation(%s,%s,%s)\n", relations[i].subject, relations[i].location, relations[i].time);

			}
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
