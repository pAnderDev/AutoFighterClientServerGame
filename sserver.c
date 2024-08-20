#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h> 
#include <pthread.h>
#include <string.h>
#include <strings.h>

/*
 *GOAL FOR SATURDAY - the fight_request method is not accepting user input on the first word. Fix that then implement a simple fighting game
 IDEAS FOR TOMORROW - try to create a while loop similar to create_character with an int called isChosen or similar = 0 to say that the receiver has not made a choice yet
 					  prompt for a choice, then accept the user input then comnpare the buffer containing the input to accept or decline then do required things with those options
 * */


#define MAX_CLIENTS 1000
void pexit(char *errmsg) {
	fprintf(stderr, "%s\n", errmsg);
	exit(1);
}

typedef struct {
	int sockfd;
	char username[200];
	int health;
	int savedHealth;
	char fightingClass[20];
	int wins; //number of duels won 
	int can_fight; //flag to determine if a player is fighting or not 0 for cant 1 for can
	char action[20];
} Client;

Client clients[MAX_CLIENTS]; //array of client player struct objects
int clientIndex = 0; //index of the client

//modular function to simulate dice rolls
int roll_dice(int num_dice, int sides) {
	int result = 0;
	for(int i = 0; i < num_dice; i++) {
		result += (rand() % sides) + 1;
	}
	return result;
}



//function to create and store and welcome connecting clients to the server
void create_character(int sockfd) {
	int valid_class = 0;
	int valid_name = 0;
	srand(time(NULL));
	//create character name
	while(!valid_name) {
		char prompt[] = "Enter your characters name: ";
		write(sockfd, prompt, strlen(prompt));

		char buffer[200];
		if(read(sockfd, buffer, sizeof(buffer)-1) <= 0) {
			printf("\nError reading from client\n");
			return;
		}
		buffer[strlen(buffer)-1] = '\0';

		//check that the name isnt already in use
		valid_name=1; //assume true unless proven not
		for(int i = 0; i < clientIndex; i++) {
			if(strcasecmp(clients[i].username, buffer) == 0) {
				char inUse[] = "Sorry, this name is taken.\n";
				write(sockfd, inUse, strlen(inUse));
				valid_name = 0;
				break;
			}
		}
	if(valid_name) {
		strcpy(clients[clientIndex].username, buffer);
		clients[clientIndex].sockfd = sockfd;

		while(!valid_class) {
			//chose character class
			char class_prompt[] = "Chose your fighting class (Warrior/Rouge/Wizard): ";
			write(sockfd, class_prompt, strlen(class_prompt));
	
			//read from client
			if(read(sockfd, buffer, sizeof(buffer)-1) <= 0) {
				printf("\nError reading class from client\n");
				return;
			}
			buffer[strlen(buffer)-1] = '\0';
	
			//check the class and set it
			if(strcasecmp(buffer, "warrior") == 0 || strcasecmp(buffer, "rouge") == 0 || strcasecmp(buffer, "wizard") == 0) {
				strcpy(clients[clientIndex].fightingClass, buffer);
				valid_class = 1;
				if(strcasecmp(buffer, "warrior") == 0) {
					int health = roll_dice(2,10);
					clients[clientIndex].health = health;
					clients[clientIndex].savedHealth = health;
					clients[clientIndex].wins = 0;
					clients[clientIndex].can_fight = 1;
				}
				else if(strcasecmp(buffer, "rouge") == 0) {
					int health = roll_dice(2,8);
					clients[clientIndex].health = health;
					clients[clientIndex].savedHealth = health;
					clients[clientIndex].wins = 0;
					clients[clientIndex].can_fight = 1;
				}
				else if(strcasecmp(buffer, "wizard") == 0) {
					int health = roll_dice(2,6);
					clients[clientIndex].health = health;
					clients[clientIndex].savedHealth = health;
					clients[clientIndex].wins = 0;
					clients[clientIndex].can_fight = 1;
				}
			} 
			else {
				char invalid_prompt[] = "Invalid option. Please pick (Warrior/Rouge/Wizard)\n";
				write(sockfd, invalid_prompt, strlen(invalid_prompt) );
			}
		}
	}
}	
	//welcome the user and explain the game
	char welcome_message[256];
	sprintf(welcome_message, "Welcome %s! the mighty %s. Your health starts at Health: %d\n", clients[clientIndex].username, clients[clientIndex].fightingClass, clients[clientIndex].health);
	write(sockfd, welcome_message, strlen(welcome_message));
	clientIndex++;
	}

//function for clients to send messages to each other
void send_message(char *sender, char *receiver, char *message) {
	char response[1024];
	int found = 0;
	sprintf(response, "[%s]: %s\n", sender, message);
	for (int i = 0; i < clientIndex; i++) {
		if(strcasecmp(clients[i].username, receiver) == 0) {
			write(clients[i].sockfd, response, strlen(response));
			found=1;
		}
	}
	if(!found) {
		char notFound[200];
		sprintf(notFound, "%s not found\n", receiver);
		write(clients[get_index(sender)].sockfd, notFound, strlen(notFound));
	}
}


//commands to process duel actions 
/*void process_action(char *first, char *second) {
	srand(time(NULL));
	char prompt[1024];
	int *first_health = clients[get_index(first)].health;
	int *second_health = clients[get_index(second)].health;
	//DAMAGE Warrior 1d8 Rouge 1d6 Wizard 1d10
	//if first player attacked and player 2 attacked | if both players attack, each makes an attack roll, AC warrior 12 AC rouge 14 AC Wizard 10
	if(strcasecmp(clients[get_index(first)].action, "attack") == 0 && strcasecmp(clients[get_index(second)].action, "attack") == 0) {
		int first_roll = roll_dice(1,20);
		int second_roll = roll_dice(1,20);

		if(first_roll > 12 && strcasecmp(clients[get_index(second)].fightingClass, "warrior") == 0 || first_roll > 14 && strcasecmp(clients[get_index(second)].fightingClass, "rouge") == 0 || first_roll > 10 && strcasecmp(clients[get_index(second)].fightingClass, "wizard") == 0) {
			int damage;
			if(strcasecmp(clients[get_index(first)].fightingClass, "warior")) {
				damage = roll_dice(1,8);
			}
			else if(strcasecmp(clients[get_index(first)].fightingClass, "rouge")) {
				damage = roll_dice(1,6);
			}
			else if(strcasecmp(clients[get_index(first)].fightingClass, "wizard")) {
				damage = roll_dice(1,10);
			}
			second_health -= damage;
			printf("%s rolled a %d to hit\n", first, first_roll);
			sprintf(prompt, "%s rolled a %d to hit! %s did %d damage to %s! %s is now at %d health.\n",first, first_roll, first, damage, second, second_health);
			write(clients[get_index(first)].sockfd, prompt, strlen(prompt));
			write(clients[get_index(second)].sockfd, prompt, strlen(prompt));
		} else {
			printf("%s rolled a %d to hit\n", first, first_roll);
			sprintf(prompt, "%s rolled a %d to hit but missed! %s is still at %d health.\n",first, first_roll, second, second_health);
		}

		if(second_roll > 12 && strcasecmp(clients[get_index(first)].fightingClass, "warrior") == 0 || first_roll > 14 && strcasecmp(clients[get_index(first)].fightingClass, "rouge") == 0 || first_roll > 10 && strcasecmp(clients[get_index(first)].fightingClass, "wizard") == 0) {
			int damage;
			if(strcasecmp(clients[get_index(second)].fightingClass, "warior")) {
				damage = roll_dice(1,8);
			}
			else if(strcasecmp(clients[get_index(second)].fightingClass, "rouge")) {
				damage = roll_dice(1,6);
			}
			else if(strcasecmp(clients[get_index(second)].fightingClass, "wizard")) {
				damage = roll_dice(1,10);
			}
			first_health -= damage;
			printf("%s rolled a %d to hit\n", second, second_roll);
			sprintf(prompt, "%s rolled a %d to hit! %s did %d damage to %s! %s is now at %d health.\n",second, second_roll, second, damage, first, first_health);
			write(clients[get_index(first)].sockfd, prompt, strlen(prompt));
			write(clients[get_index(second)].sockfd, prompt, strlen(prompt));
		} else {
			printf("%s rolled a %d to hit\n", second, second_roll);
			sprintf(prompt, "%s rolled a %d to hit but missed! %s is still at %d health.\n",second, second_roll, first, first_health);
			write(clients[get_index(first)].sockfd, prompt, strlen(prompt));
			write(clients[get_index(second)].sockfd, prompt, strlen(prompt));
			
		}
	}
	//if player 1 attack and player 2 defend
	else if(strcasecmp(clients[get_index(first)].action, "attack") == 0 && strcasecmp(clients[get_index(second)].action, "attack") == 0) {
		
	}

	//if player 1 defend and player 2 attack
	else if(strcasecmp(clients[get_index(first)].action, "attack") == 0 && strcasecmp(clients[get_index(second)].action, "attack") == 0) {
		
	}
		
	//if player 1 defend and player 2 defend
	else if(strcasecmp(clients[get_index(first)].action, "attack") == 0 && strcasecmp(clients[get_index(second)].action, "attack") == 0) {
		
	} 
	else {
		printf("\nFailed to process duel actions\n");
		return;
	}

}*/

void reset_health(char *player) {
	printf("reset health\n");
	int reset = clients[get_index(player)].savedHealth;
	clients[get_index(player)].health = reset;
}
//testing makle!!!

void declare_winner(char *winner, char *loser) {
	char prompt[1024];
	printf("Processing winner: %s winner %s loser", winner, loser);
	sleep(2);
	sprintf(prompt, "%s has won the duel! %s has been defeated\n", winner, loser);
	write(clients[get_index(winner)].sockfd, prompt, strlen(prompt));
	write(clients[get_index(loser)].sockfd, prompt, strlen(prompt));

	//increase the win and health of the winner
	clients[get_index(winner)].savedHealth += 1;
	clients[get_index(winner)].wins += 1;
	
	//reset can fight flah
	
	clients[get_index(winner)].can_fight = 1;
	clients[get_index(loser)].can_fight = 1;
	
	reset_health(winner);
	reset_health(loser);
	

}

//this is the function to control game flow
void duel(char *s, char *r) {
	srand(time(NULL));
	char prompt[1024];
	char *first;
	char *second;
	int sHealth = clients[get_index(s)].savedHealth;
	int rHealth = clients[get_index(r)].savedHealth;
	//first roll to see who goes first
	int s_roll = roll_dice(1,20);
	printf("%s rolled %d for turn order\n", s, s_roll);
	int r_roll = roll_dice(1,20);
	printf("%s rolled %d for turn order\n", r, r_roll);
	if(s_roll > r_roll) {
		sprintf(prompt, "%s rolled higher (%d > %d) so they will go first!\n", s, s_roll, r_roll);
		write(clients[get_index(s)].sockfd, prompt, strlen(prompt));
		write(clients[get_index(r)].sockfd, prompt, strlen(prompt));
		first = s;
		second = r;
	} else {
		sprintf(prompt, "%s rolled higher (%d > %d) so they will go first!\n", r, r_roll, s_roll);
		write(clients[get_index(s)].sockfd, prompt, strlen(prompt));
		write(clients[get_index(r)].sockfd, prompt, strlen(prompt));
		first = r;
		second = s;
	}
	int round = 1;
	int winner = 0;
	while(!winner) {
		int attack_roll_first = roll_dice(1,20);
		int defend_roll_second = roll_dice(1,20);
		sleep(1);
		printf("--- ROUND %d ---\n%s rolled attack (%d)\n%s rolled defend (%d)\n", round, first, attack_roll_first, second, defend_roll_second);
		sprintf(prompt, "--- ROUND %d ---\n%s rolled attack (%d)\n%s rolled defend (%d)\n", round, first, attack_roll_first, second, defend_roll_second);
		write(clients[get_index(first)].sockfd, prompt, strlen(prompt));
		write(clients[get_index(second)].sockfd, prompt, strlen(prompt));
		
		//first player attacks second defends
		sleep(1);
		if(attack_roll_first > defend_roll_second) {
			if(strcasecmp(clients[get_index(first)].fightingClass, "Warrior")) {
				int damage = roll_dice(1,6);
				clients[get_index(second)].health -= damage;
				sprintf(prompt, "%s slashed at %s and did %d damage!\n", first, second, damage);
				printf("%s Health: %d\n %s Health: %d\n", first, clients[get_index(first)].health, second, clients[get_index(second)].health);
				write(clients[get_index(first)].sockfd, prompt, strlen(prompt));
				write(clients[get_index(second)].sockfd, prompt, strlen(prompt));
			}
			else if(strcasecmp(clients[get_index(first)].fightingClass, "Rouge")) {
				int damage = roll_dice(1,8);
				clients[get_index(second)].health -= damage;
				sprintf(prompt, "%s sneakily stabbed at %s and did %d damage!\n", first, second, damage);
				write(clients[get_index(first)].sockfd, prompt, strlen(prompt));
				write(clients[get_index(second)].sockfd, prompt, strlen(prompt));
			}
			else if(strcasecmp(clients[get_index(first)].fightingClass, "Wizard")) {
				int damage = roll_dice(1,10);
				clients[get_index(second)].health -= damage;
				sprintf(prompt, "%s cast a powerful spell at %s and did %d damage!\n", first, second, damage);
				write(clients[get_index(first)].sockfd, prompt, strlen(prompt));
				write(clients[get_index(second)].sockfd, prompt, strlen(prompt));
			}
			memset(prompt, 0, sizeof(prompt));
			if(clients[get_index(second)].health <= 0) {
				declare_winner(first, second);
				winner = 1;
				return;
			}
		}
		else {
			if(strcasecmp(clients[get_index(second)].fightingClass, "Warrior")) {
				sprintf(prompt, "%s sheild absored the damage from %s!\n", second, first);
				write(clients[get_index(first)].sockfd, prompt, strlen(prompt));
				write(clients[get_index(second)].sockfd, prompt, strlen(prompt));
			}
			else if(strcasecmp(clients[get_index(second)].fightingClass, "Rouge")) {
				sprintf(prompt, "%s quickly dodge the damage from %s!\n",second, first);
				write(clients[get_index(first)].sockfd, prompt, strlen(prompt));
				write(clients[get_index(second)].sockfd, prompt, strlen(prompt));
			}
			else if(strcasecmp(clients[get_index(second)].fightingClass, "Wizard")) {
				sprintf(prompt, "%s cast a protective barrier to protect from %s!\n", second, first);
				write(clients[get_index(first)].sockfd, prompt, strlen(prompt));
				write(clients[get_index(second)].sockfd, prompt, strlen(prompt));
			}
		}
		round++;
		int attack_roll_second = roll_dice(1,20);
		int defend_roll_first = roll_dice(1,20);
		sleep(1);
		printf("--- ROUND %d ---\n%s rolled attack (%d)\n %s rolled defend (%d)\n", round, second, attack_roll_second, first, defend_roll_first);
		sprintf(prompt, "--- ROUND %d ---\n%s rolled attack (%d)\n %s rolled defend (%d)\n", round, second, attack_roll_second, first, defend_roll_first);
		write(clients[get_index(first)].sockfd, prompt, strlen(prompt));
		write(clients[get_index(second)].sockfd, prompt, strlen(prompt));
		
		//first player attacks second defends
		sleep(1);
		if(attack_roll_second > defend_roll_first) {
			if(strcasecmp(clients[get_index(second)].fightingClass, "Warrior")) {
				int damage = roll_dice(1,6);
				clients[get_index(first)].health -= damage;
				sprintf(prompt, "%s slashed at %s and did %d damage!\n", second, first, damage);
				write(clients[get_index(first)].sockfd, prompt, strlen(prompt));
				write(clients[get_index(second)].sockfd, prompt, strlen(prompt));
			}
			else if(strcasecmp(clients[get_index(second)].fightingClass, "Rouge")) {
				int damage = roll_dice(1,8);
				clients[get_index(first)].health -= damage;
				sprintf(prompt, "%s sneakily stabbed at %s and did %d damage!\n", second, first, damage);
				write(clients[get_index(first)].sockfd, prompt, strlen(prompt));
				write(clients[get_index(second)].sockfd, prompt, strlen(prompt));
			}
			else if(strcasecmp(clients[get_index(second)].fightingClass, "Wizard")) {
				int damage = roll_dice(1,10);
				clients[get_index(first)].health -= damage;
				sprintf(prompt, "%s cast a powerful spell at %s and did %d damage!\n", second, first, damage);
				write(clients[get_index(first)].sockfd, prompt, strlen(prompt));
				write(clients[get_index(second)].sockfd, prompt, strlen(prompt));
			}
			memset(prompt, 0, sizeof(prompt));
			if(clients[get_index(first)].health <= 0) {
				declare_winner(second, first);
				winner=1;
				return;
			}
		}
		else {
			if(strcasecmp(clients[get_index(second)].fightingClass, "Warrior")) {
				sprintf(prompt, "%s sheild absored the damage from %s!\n", first, second);
				write(clients[get_index(first)].sockfd, prompt, strlen(prompt));
				write(clients[get_index(second)].sockfd, prompt, strlen(prompt));
			}
			else if(strcasecmp(clients[get_index(second)].fightingClass, "Rouge")) {
				sprintf(prompt, "%s quickly dodge the damage from %s!\n", first, second);
				write(clients[get_index(first)].sockfd, prompt, strlen(prompt));
				write(clients[get_index(second)].sockfd, prompt, strlen(prompt));
			}
			else if(strcasecmp(clients[get_index(second)].fightingClass, "Wizard")) {
				sprintf(prompt, "%s cast a protective barrier to protect from %s!\n", first, second);
				write(clients[get_index(first)].sockfd, prompt, strlen(prompt));
				write(clients[get_index(second)].sockfd, prompt, strlen(prompt));
			}
		}
		round++;

	}
}


//function called to send duel request to other players - leads into playing the game
void fight_request(char *sender, char *receiver) {
	int found = 0;
	int chose = 0;
	char prompt[200];
	char buffer[20];
	while(!found) { //while the receiver has not be found yet
		//look for the receiver
			if(clients[get_index(receiver)].can_fight == 1) {
				//send the inital Duel request
				found = 1;
				sprintf(prompt,"[%s]: Lets Duel %s! What say you? | Enter Input Twice! | (accept/decline): \n", sender, receiver);
				write(clients[get_index(receiver)].sockfd, prompt, strlen(prompt));
				clients[get_index(receiver)].can_fight = 0;
				clients[get_index(sender)].can_fight = 0;
				memset(prompt, 0, sizeof(prompt));

			}
		//if the player is found and can fight - then read their response
		if(found) {
			//while the player has not accepted or declined
			while(!chose) {
				//read their response
				chose = 1;
				memset(buffer, 0, sizeof(buffer));
				memset(prompt, 0, sizeof(prompt));
				if(read(clients[get_index(receiver)].sockfd, buffer, sizeof(buffer)-1) <= 0) {
					printf("\nError reading from %s\n", receiver);
					return;
				}
				buffer[strlen(buffer)-1] = '\0';
				printf("Duel request choice: %s chose %s\n",receiver, buffer);
				if(strcasecmp(buffer, "accept") == 0) {
					memset(prompt, 0, sizeof(prompt));
					sprintf(prompt, "[%s]: Lets duel!\n", receiver);
					printf("%s accepted duel from %s\n",receiver, sender );
					write(clients[get_index(sender)].sockfd, prompt, sizeof(prompt));
					duel(sender, receiver);
				}
				else {
					memset(prompt, 0, sizeof(prompt));
					sprintf(prompt, "[%s]: Has declined the duel\n", receiver);
					write(clients[get_index(sender)].sockfd, prompt, strlen(prompt));
					clients[get_index(receiver)].can_fight = 1;
					clients[get_index(sender)].can_fight = 1;
				}
			memset(buffer, 0, sizeof(buffer));
			}

		}
		if(!found) {
			memset(prompt, 0, sizeof(prompt));
			sprintf(prompt, "%s not found or is unavailable. Please do <list> to see available players.\n", receiver);
			write(clients[get_index(sender)].sockfd, prompt, strlen(prompt));
			return;
		}
	}
}

//function to return the index of a specific user/thread
int get_index(char *username) {
	for(int i = 0; i < clientIndex; i++) {
		if(strcmp(clients[i].username, username) == 0) {
			return i;
		}
	}
	return -1;
}

//function to list all the users
void list_users(int sockfd) {
	char response[1024];
	sprintf(response, "Connected users:\n");
	//loop through and list all players and relevant information
	for(int i = 0; i < clientIndex; i++) {
		sprintf(response + strlen(response), "%s (%s) Health: %d Wins: %d - %s\n", clients[i].username, clients[i].fightingClass, clients[i].health, clients[i].wins, clients[i].can_fight ? "Available" : "Unavailable");
	}
	write(sockfd, response, strlen(response));
	memset(response, 0, sizeof(response));
	return;
}

void check_user(int sockfd, char *user) {
	char response[1024];
	memset(response, 0, sizeof(response));
	//loop through and list all players and relevant information
		sprintf(response + strlen(response), "%s (%s) Health: %d Wins: %d - %s\n", clients[get_index(user)].username, clients[get_index(user)].fightingClass, clients[get_index(user)].health, clients[get_index(user)].wins, clients[get_index(user)].can_fight ? "Available" : "Unavailable");
	write(sockfd, response, strlen(response));
	return;
}
//function to list all available commands
void help_list(int sockfd) {
	char response[1024];
	memset(response, 0, sizeof(response));
	sprintf(response, "Here is the list of the commands:\n | send : send <user> <message> | send a message to a specific user \n | list : list | list all players \n | check : check <user> | check the avalibilty of a player \n | duel : duel <user> | send a duel request to specific user \n | help : help | open list of commands\n");
	write(sockfd, response, strlen(response));
	return;
}

//function for dedicated server thread
void *child(void *ptr) {
	uint32_t connfd = (uint32_t) ptr; //sets connfd to the connect file descriptor of the thread
	int currIndex = clientIndex; //sets currIndex to the connfd of the current thread
	clients[currIndex].sockfd = connfd; //sets the Client sockfd of this thread and specific client
	create_character(connfd); //run command to create username
		
	//loop to accept input and run associated commands
	while (1) {
		char buffer[1024];		
		memset(buffer, 0, sizeof(buffer));
		if(read(connfd, buffer, sizeof(buffer)-1) <= 0) {
			printf("\n Read from Client Error \n");
			break;
		}
		
		size_t len = strlen(buffer);
		if(len > 0 && buffer[len-1] == '\n') {
			buffer[len-1] = '\0';
		}
		
		//token for user input
		char *token = strtok(buffer, " ");
		if(token != NULL) {
			//if the user types send - send a message to another user | send user message
			if(strcasecmp(token, "send")==0) {
				char *receiver = strtok(NULL, " ");
				char *message = strtok(NULL, "");
				if(receiver != NULL && message != NULL) {
					printf("Proecessing send : to %s from %s at index %d\n", receiver, clients[currIndex].username, currIndex);
					send_message(clients[currIndex].username, receiver, message);
				} 
				//if the command is not typed correctly
				else {
					char invalid_prompt[] = "Invalid syntax for command 'send' (send <user> <message>)\n";
					write(connfd, invalid_prompt, sizeof(invalid_prompt));
				}
			}
			//if the user types list, run list_users function 
			else if(strcasecmp(token, "list") == 0) {
				list_users(connfd);
			}
			//if user types duel user then run fight_request function
			else if(strcasecmp(token, "duel") == 0) {
				char *receiver = strtok(NULL, " ");
				if(receiver != NULL) {
					printf("Processing duel : to %s from %s at index %d\n", receiver, clients[currIndex].username, currIndex);
					char prompt[200];
					sprintf(prompt, "When dueling make sure to press enter first before your input\n");
					write(connfd, prompt, strlen(prompt));
					fight_request(clients[currIndex].username, receiver);
				} 
				//if the syntax of the command is not correct
				else {
					char invalid_prompt[] = "Invalid syntax for command 'duel' (duel <user)>\n";
					write(connfd, invalid_prompt, sizeof(invalid_prompt));
				}
			}
			else if(strcasecmp(token, "check") == 0) {
				char *user = strtok(NULL, " ");
				if(user != NULL) {
					printf("Processing check user %s\n", user);
					check_user(connfd, user);
				}
				else {
					char invalid_prompt[] = "Invalid syntax for command 'check' (check <user)>\n";
					write(connfd, invalid_prompt, sizeof(invalid_prompt));
				}
			}
			//if user types help, run help function to list all commands
			else if(strcasecmp(token, "help") == 0) {
				printf("Processing help command\n");
				help_list(connfd);
			}
		}
	}
	close(connfd);
	clientIndex--;
	pthread_exit(NULL);
}

int main(int argc, char *argv[])
{

    int listenfd = 0, connfd = 0;
    struct sockaddr_in serv_addr; 

    char buffer[1025];
    time_t ticks; 

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		pexit("socket() error.");
		
    memset(&serv_addr, '0', sizeof(serv_addr));
    memset(buffer, '0', sizeof(buffer)); 

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	int port = 4999;
	do {
		port++;
    	serv_addr.sin_port = htons(port); 
    } while (bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0);
	printf("bind() succeeds for port #%d\n", port);

    if (listen(listenfd, 10) < 0)
		pexit("listen() error.");

	int counter=0;
    while(1)
    {
        connfd = accept(listenfd, (struct sockaddr*)NULL, NULL); 
		counter++;
		fprintf(stderr, "connected to client %d.\n", counter);
	
		pthread_t thread1;
        pthread_create(&thread1, NULL, child, (void *) connfd);

     }
}




/*	srand(time(NULL));
	char buffer[50];
	char prompt[100];
	char *first;
	char *second;
	memset(buffer, 0, sizeof(buffer));
	memset(prompt, 0, sizeof(prompt));
	//roll d100 see who goes first
	int s_roll = roll_dice(1, 100);
	int r_roll = roll_dice(1, 100);

	//DETERMINE TURN ORDER
	//if sender rolled higher in turn order picking
	if(s_roll > r_roll) {
		sprintf(prompt, "%s rolled higher(%d > %d) and choses who goes first: %s or %s\n", s, s_roll, r_roll, s, r);
		memset(prompt, 0, sizeof(prompt));
		write(clients[get_index(s)].sockfd, prompt, strlen(prompt));
		write(clients[get_index(r)].sockfd, prompt, strlen(prompt));
		//read response
		if(read(clients[get_index(s)].sockfd, buffer, sizeof(buffer)-1) <= 0) {
			printf("\nError reading from client\n");
			return;
		}
		buffer[strlen(buffer)-1] = '\0';
		//if sender chosen to go first
		if(strcasecmp(buffer, s) == 0) {
			first = s;
			second = r;
			sprintf(prompt, "%s has chosen %s to go first! %s will go second\n", s, s, r);
			write(clients[get_index(s)].sockfd, prompt, strlen(prompt));
			write(clients[get_index(r)].sockfd, prompt, strlen(prompt));
			memset(prompt, 0, sizeof(prompt)); //clear chose buffer
		}
		else if(strcasecmp(buffer, r) == 0) {
			first = r;
			second = s;
			sprintf(prompt, "%s has chosen %s to go first! %s will go second\n", s, r, s);
			write(clients[get_index(s)].sockfd, prompt, strlen(prompt));
			write(clients[get_index(r)].sockfd, prompt, strlen(prompt));
			memset(prompt, 0, sizeof(prompt)); //clear chose buffer
		}
	}
	//if receiver rolled higher in turn order picking
	else if (r_roll > s_roll) {
		memset(prompt, 0, sizeof(prompt)); //clear chose buffer
		sprintf(prompt, "%s rolled higher(%d > %d) and choses who goes first: %s or %s\n", r, r_roll, s_roll, s, r);
		write(clients[get_index(s)].sockfd, prompt, strlen(prompt));
		write(clients[get_index(r)].sockfd, prompt, strlen(prompt));
	//read response
		if(read(clients[get_index(r)].sockfd, buffer, sizeof(buffer)-1) <= 0) {
			printf("\nError reading from client\n");
			return;
		}
		buffer[strlen(buffer)-1] = '\0';
		//if sender chosen to go first
		if(strcasecmp(buffer, s) == 0) {
			first = s;
			second = r; 
			sprintf(prompt, "%s has chosen %s to go first! %s will go second\n", r, s, r);
			write(clients[get_index(s)].sockfd, prompt, strlen(prompt));
			write(clients[get_index(r)].sockfd, prompt, strlen(prompt));
			memset(prompt, 0, sizeof(prompt)); //clear chose buffer
		}
		else if(strcasecmp(buffer, r) == 0) {
			first = r;
			second = s;
			sprintf(prompt, "%s has chosen %s to go first! %s will go second\n", r, r, s);
			write(clients[get_index(s)].sockfd, prompt, strlen(prompt));
			write(clients[get_index(r)].sockfd, prompt, strlen(prompt));
			memset(prompt, 0, sizeof(prompt)); //clear chose buffer
		}
	}

	//begin game lopp
	int round = 1;
	while(clients[get_index(first)].health > 0 && clients[get_index(second)].health > 0) {
		char prompt[100];
		char *current_player;
		char *next_player;
		if(round % 2 == 1) {
			current_player = first;
			next_player = second;
		} else {
			current_player = second;
			next_player = first;
		}
		int valid = 0;
		while(!valid) {
			sprintf(prompt, "Round %d %s's first! | Remember to Press Enter before your input!| (attack/defend)\n", round, current_player);
			write(clients[get_index(current_player)].sockfd, prompt, strlen(prompt));
			write(clients[get_index(next_player)].sockfd, prompt, strlen(prompt));
	
			sprintf(prompt, "Waiting for %'s to make their move...\n", current_player);
			write(clients[get_index(next_player)].sockfd, prompt, strlen(prompt));
	
			//reset prompt and buffer
			memset(prompt, 0, sizeof(prompt));
			memset(buffer, 0, sizeof(buffer));
	
			if(read(clients[get_index(current_player)].sockfd, buffer, sizeof(buffer)) <= 0) {
				printf("\Error reading from clients\n");
				return;
			}
			buffer[strlen(buffer)-1] = '\0';
			//proccess the action of the current player
			if(strcasecmp(buffer, "attack") == 0 || strcasecmp(buffer, "defend") == 0) {
				valid=1;
				strcpy(clients[get_index(current_player)].action, buffer);
				printf("Player 1 Action: %s\n", clients[get_index(current_player)].action);
			} else {
				char commands[40];
				sprintf(commands, "Please enter a valid action: attack, defend\n");
				write(clients[get_index(current_player)].sockfd, commands, strlen(commands));
			}
		}
	int valid_two = 0;
		while(!valid_two) {
			//if the first player has made their choice - player 2 now choses an action
			if(clients[get_index(current_player)].action[0] != '\0') {		
				sprintf(prompt, "Waiting for %'s to make their move...\n", next_player);
				write(clients[get_index(current_player)].sockfd, prompt, strlen(prompt));
			
				sprintf(prompt, "Round %d %s's turn! | Remember to Press Enter before your input!| (attack/defend)\n", round, next_player);
				write(clients[get_index(next_player)].sockfd, prompt, strlen(prompt));
			
				memset(prompt, 0, sizeof(prompt));
				memset(buffer, 0, sizeof(buffer));
	
				if(read(clients[get_index(next_player)].sockfd, buffer, sizeof(buffer)-1) <= 0) {
					printf("\Error reading from clients\n");
					return;
				}
				buffer[strlen(buffer)-1] = '\0';
		
				printf("Action in buffer: %s\n", buffer);	
				//proccess the action of the next player
				if(strcasecmp(buffer, "attack") == 0 || strcasecmp(buffer, "defend") == 0) {
					valid_two=1;
					strcpy(clients[get_index(next_player)].action, buffer);
					printf("Player 2 Action: %s\n", clients[get_index(next_player)].action);
				} else {
					char commands[40];
					memset(commands, 0, sizeof(commands));
					sprintf(commands, "Please enter a valid action: attack, defend\n");
					write(clients[get_index(next_player)].sockfd, commands, strlen(commands));
				}
			}
		}
		
		//both players have chosen an action
		if(clients[get_index(first)].action[0] != '\0' && clients[get_index(second)].action[0] != '\0') {
			process_action(first, second);

			strcpy(clients[get_index(first)].action, "");
			strcpy(clients[get_index(second)].action, "");

			round++;

		}
	}*/



//goes inside duel function - doesnt work
	/*int round = 1;
		//first persons turn
		if(round % 2 == 1) {
			int attack_roll_first = roll_dice(1,20);
			printf("%s rolled %d to attack\n", first, attack_roll_first);
			int defence_roll_second = roll_dice(1,20);
			printf("%s rolled %d to defend\n", second, defence_roll_second);
			
			round++;
			
			printf("Round %d:\n%s: Health - %d \n%s: Health - %d \n", round, first, clients[get_index(first)].health , second);
			sprintf(prompt, "Round %d:\n%s: Health - %d \n%s: Health - %d \n", round, first, , second);
			write(clients[get_index(first)].sockfd, prompt, strlen(prompt));
			write(clients[get_index(second)].sockfd, prompt, strlen(prompt));
			
			memset(prompt, 0, sizeof(prompt));

			if(attack_roll_first > defence_roll_second) {
				if(strcasecmp(clients[get_index(first)].fightingClass, "Warrior")) {
					int damage = roll_dice(1,6);
					clients[get_index(second)].health -= damage;
					sprintf(prompt, "%s rolled an attack (%d) which was higher than %s defence roll (%d)! %s did %d damage to %s.\n Current Health: %s %d\n %s %d\n", first, attack_roll_first, second, defence_roll_second, first, damage, second, first, clients[get_index(first)].health, second, clients[get_index(second)].health);
					write(clients[get_index(first)].sockfd, prompt, strlen(prompt));
					write(clients[get_index(second)].sockfd, prompt, strlen(prompt));

				}
				if(strcasecmp(clients[get_index(first)].fightingClass, "Rouge")) {
					int damage = roll_dice(1,8);
					clients[get_index(second)].health -= damage;
					sprintf(prompt, "%s rolled an attack (%d) which was higher than %s defence roll (%d)! %s did %d damage to %s.\n Current Health: %s %d\n %s %d\n", first, attack_roll_first, second, defence_roll_second, first, damage, second, first, clients[get_index(first)].health, second, clients[get_index(second)].health);
					write(clients[get_index(first)].sockfd, prompt, strlen(prompt));
					write(clients[get_index(second)].sockfd, prompt, strlen(prompt));
				}
				if(strcasecmp(clients[get_index(first)].fightingClass, "Wizard")) {
					int damage = roll_dice(1,10);
					clients[get_index(second)].health -= damage;
					sprintf(prompt, "%s rolled an attack (%d) which was higher than %s defence roll (%d)! %s did %d damage to %s.\n Current Health: %s %d\n %s %d\n", first, attack_roll_first, second, defence_roll_second, first, damage, second, first, clients[get_index(first)].health, second, clients[get_index(second)].health);
					write(clients[get_index(first)].sockfd, prompt, strlen(prompt));
					write(clients[get_index(second)].sockfd, prompt, strlen(prompt));
				}
			}	
			else if(attack_roll_first < defence_roll_second) {
					round++;
					sprintf(prompt, "%s rolled an attack (%d) which was missed %s!(%d).\n Current Health: %s %d\n %s %d\n", first, attack_roll_first, second, defence_roll_second, first, clients[get_index(first)].health, second, clients[get_index(second)].health);
					write(clients[get_index(first)].sockfd, prompt, strlen(prompt));
					write(clients[get_index(second)].sockfd, prompt, strlen(prompt));
			}
			if(clients[get_index(second)].health <= 0) {
				declare_winner(first, second);
				return;
			}
		}
	/*	else if(round % 2 == 0) {
			memset(prompt, 0, sizeof(prompt));
			int attack_roll_second = roll_dice(1,20);
			printf("%s rolled %d to attack\n", second, attack_roll_second);
			int defence_roll_first = roll_dice(1,20);
			printf("%s rolled %d to defend\n", second, defence_roll_first);
			
			printf("Round %d: %s attack %s defend\n", round, second, first);
			sprintf(prompt, "Round %d: %s attack %s defend\n", round, second, first);
			write(clients[get_index(first)].sockfd, prompt, strlen(prompt));
			write(clients[get_index(second)].sockfd, prompt, strlen(prompt));
			memset(prompt, 0, sizeof(prompt));
			if(attack_roll_second > defence_roll_first) {
				round++;
				if(strcasecmp(clients[get_index(second)].fightingClass, "Warrior")) {
					int damage = roll_dice(1,6);
					clients[get_index(first)].health -= damage;
					sprintf(prompt, "%s rolled an attack (%d) which was higher than %s defence roll (%d)! %s did %d damage to %s.\n Current Health: %s %d\n %s %d\n", second, attack_roll_second, first, defence_roll_first, second, damage, first, first, clients[get_index(first)].health, second, clients[get_index(second)].health);
					write(clients[get_index(first)].sockfd, prompt, strlen(prompt));
					write(clients[get_index(second)].sockfd, prompt, strlen(prompt));

				}
				if(strcasecmp(clients[get_index(second)].fightingClass, "Rouge")) {
					int damage = roll_dice(1,8);
					clients[get_index(first)].health -= damage;
					sprintf(prompt, "%s rolled an attack (%d) which was higher than %s defence roll (%d)! %s did %d damage to %s.\n Current Health: %s %d\n %s %d\n", second, attack_roll_second, first, defence_roll_first, second, damage, first, first, clients[get_index(first)].health, second, clients[get_index(second)].health);
					write(clients[get_index(first)].sockfd, prompt, strlen(prompt));
					write(clients[get_index(second)].sockfd, prompt, strlen(prompt));
				}
				if(strcasecmp(clients[get_index(second)].fightingClass, "Wizard")) {
					int damage = roll_dice(1,10);
					clients[get_index(first)].health -= damage;
					sprintf(prompt, "%s rolled an attack (%d) which was higher than %s defence roll (%d)! %s did %d damage to %s.\n Current Health: %s %d\n %s %d\n", second, attack_roll_second, first, defence_roll_first, second, damage, first, first, clients[get_index(first)].health, second, clients[get_index(second)].health);
					write(clients[get_index(first)].sockfd, prompt, strlen(prompt));
					write(clients[get_index(second)].sockfd, prompt, strlen(prompt));
				}
			}	
			else if(attack_roll_second < defence_roll_first) {
					round++;
					sprintf(prompt, "%s rolled an attack (%d) which was missed %s!(%d).\n Current Health: %s %d\n %s %d\n", second, attack_roll_second, first, defence_roll_first, first, clients[get_index(first)].health, second, clients[get_index(second)].health);
					write(clients[get_index(first)].sockfd, prompt, strlen(prompt));
					write(clients[get_index(second)].sockfd, prompt, strlen(prompt));
			}
			if(clients[get_index(first)].health <= 0) {
				declare_winner(second, first);
				return;
			}
		}*/

