#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORTNUM 9000
#define IPADDR "127.0.0.1"
#define HAND 11

typedef struct {
	int suit[HAND];
	int number[HAND];
	int count;
	int draw;
	int bust;
} Card;

Card Client_Card, Server_Card;
int client_fd;
int balance;
int bet;
int result;//결과를 저장하기 위한 변수이다. 

void GameIntro();
void ShowBalance();
void ShowInfo();
void ReceiveCard();
void ErrorCheck(int num, char* str);
void ConnectToServer();
void SendBalance();
void SendClientBet();
void ReceiveServerCard();
void ReceiveResult();
void ClientPrintHand();
void ServerPrintHand();
int AskForRetry();
void SendClientDraw();
void ReceiveBet();

int main(int argc, char* argv[]) {

	system("clear");
	ConnectToServer();//server와 통신을 설정하는 함수이다. 

	GameIntro();//게임 시작 화면을 출력하는 함수이다. 
	SendBalance();//초기 시작 소지금을 입력해 server로 전송하는 함수이다. 

	do {//send와 recv가 번갈아 가며 진행되기 때문에 가능한 모든 경우에 수를 모두 예외처리해주었다. 
		system("clear");
		SendClientBet();//게임이 시작되고 베팅금액을 입력받아 서버로 전송한다. 
		Client_Card.draw = 1;//초기 동작을 위해 draw와 bust를 1로 저장한다. 
		Client_Card.bust = 1;
		do {
			ShowInfo();//현재 소지금과 베팅 금액을 출력한다. 
			if (!Client_Card.draw) break;//client가 더이상 draw를 하지 않고 싶으면 break한다. 
			ReceiveCard();//server로보터 뽑은 카드를 수신한다. 
			ClientPrintHand();//client가 그동안 뽑은 카드를 출력한다. 
			if (!Client_Card.bust) break;//client가 이번에 뽑은 카드로 인해 bust면 break한다. 
			if (!Client_Card.draw) break;
			SendClientDraw();
		} while (Client_Card.draw);//clientcard.draw가 더이상 카드를 뽑고 싶지 않을때까지 반복한다. 
		ShowInfo();//화면을 초기화하고 정보를 출력한다. 
		ReceiveServerCard();//결과를 출력하기 위해 server 카드를 수신한다. 
		ClientPrintHand();//client와 server가 그동안 뽑은 카드를 출력한다. 
		ServerPrintHand();
		ReceiveResult();//결과를 수신한다. 
		ReceiveBet();//해당 결과로 변경된 balance를 수신한다. 
	} while (AskForRetry());//client에 다른판을 진행할껀지 묻고 해당 값에 따라 loop를 탈출할지 다시 진행할지 결정된다. 

	printf("\nFinally, your remaining budget is %d.\n", balance);//게임이 끝난 이후 client의 남은 소지금을 출력한다. 

	close(client_fd);
	return 0;
}

void ConnectToServer() {
	struct sockaddr_in server_addr;


	client_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	ErrorCheck(client_fd, "Socket Creation");


	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORTNUM);
	server_addr.sin_addr.s_addr = inet_addr(IPADDR);

	ErrorCheck(connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)), "Connect");
}

void GameIntro() {//화면 초기화 하고 초기 환영메시지를 출력한다. 
	system("clear");
	printf("******************************************\n");
	printf("*                                        *\n");
	printf("*            Blackjack Game              *\n");
	printf("*                                        *\n");
	printf("*      Welcome to the Blackjack game.    *\n");
	printf("* Enjoy Blackjack with dealers (servers).*\n");
	printf("*                                        *\n");
	printf("******************************************\n");
}

void SendBalance() {//client에서 받은 소지금을 server로 전송하고 balance를 출력한다. 
	printf("Enter yout budget: ");
	scanf("%d", &balance);
	ssize_t sent_bytes = send(client_fd, &balance, sizeof(balance), 0);
	ErrorCheck(sent_bytes, "Send Balance");
	fflush(stdout);
}

void ShowBalance() {//입력받은 소지금을 출력한다. 상단에 위치하고 화면 초기화하기 때문에 bet이 balance를 넘어가는지 확인하고 출력하는문을 이 함수에 배치하였다. 
	system("clear");
	printf("******************************************\n");
	printf("*                                        *\n");
	printf("*          Your budget is %d.           *\n", balance);
	printf("*                                        *\n");
	printf("******************************************\n");
	if (bet > balance) printf("Bet amount is over your budget.\n");
}

void ShowInfo() {//현재 소지금과 배팅금액을 출력한다. 
	system("clear");
	printf("******************************************\n");
	printf("*                                        *\n");
	printf("*          Your budget is %d.           *\n", balance);
	printf("*   Betting amount for this game is %d.  *\n", bet);
	printf("*                                        *\n");
	printf("******************************************\n\n");
}

void SendClientBet() {//client에서 server로 배팅 금액을 보내는 함수로 베팅 금액이 소지금을 넘어가면 다시 물어보도록 한다. 
	int over = 0;
	bet = 0;
	do {
		ShowBalance();
		printf("Enter the amount to bet on this game: ");
		scanf("%d", &bet);//베팅금을 저장한다. 
		if (bet > balance) over = 1;//베팅금이 소지금을 넘어가면 다시 입력 받는다. 
		else break;
	} while (over);
	ssize_t sent_bytes = send(client_fd, &bet, sizeof(bet), 0);
	ErrorCheck(sent_bytes, "Send Bet");
}

void ReceiveCard() {//server로 부터 뽑은 카드 정보를 수신한다.  client에서 카드를 뽑지 않은 이유는 게임을 만들때 client에서 뽑게 시키면 변조의 위험이 있기때문에 server에서 카드를 넘겨 받는 구조로 설계했다. 
	ssize_t received_bytes;
	do {
		received_bytes = recv(client_fd, &Client_Card, sizeof(Client_Card), 0);
	} while (!received_bytes);
}

void ReceiveServerCard() {//한판 종료후 결과를 알기 위해 server의 card를 수신한다. 
	ssize_t received_bytes;
	while (1) {
		received_bytes = recv(client_fd, &Server_Card, sizeof(Server_Card), 0);
		if (received_bytes > 0) break;
	}
}

void ReceiveResult() {//결과를 수신하여 client 승무패에 따라서 결과를 출력한다. 
	ssize_t received_bytes = recv(client_fd, &result, sizeof(result), 0);
	ErrorCheck(received_bytes, "Receive Result");
	printf("=========================================\n");
	printf("||             Game Result             ||\n");
	switch (result) {
	case 2: printf("||             You Win!                ||\n"); break;
	case 1: printf("||             You Lose!               ||\n"); break;
	case 0: printf("||              Draw!                  ||\n"); break;
	default: printf("||           Unknown Result              ||\n"); break;
	}
	printf("=========================================\n\n");
}

void ReceiveBet() {//게임 종료 이후 승패에 따른 변동된 소지금을 수신한다. 
	ssize_t received_bytes = recv(client_fd, &balance, sizeof(result), 0);
}

void SendClientDraw() {
	int stat;
	printf("=========================================\n");
	printf("||                1.Hit                ||\n");
	printf("||                0.Stand              ||\n");
	printf("=========================================\n");
	printf("-> ");
	scanf("%d", &stat);
	ssize_t sent_bytes = send(client_fd, &stat, sizeof(stat), 0);
	ErrorCheck(sent_bytes, "Send Draw Request");
	if (!stat) Client_Card.draw = 0;
}

int AskForRetry() {//다시 할지 물어보는 함수로 retry변수에 저장하여 retry값을 server로 전송하고 retry값을 return한다. 
	int retry;
	if (balance < 1) {//소지금이 0이면 아래 문자열을 출력하고 0을 return하여 게임을 종료한다. 
		printf("Exhausted all the budget you had\n");
		return 0;
	}//다시 할지 묻고 현재 소지금을 출력한다. 
	printf("Your remaining budget is %d$.\nDo you want to play more? (1. yes, 0. no): ", balance);
	scanf("%d", &retry);
	ssize_t sent_bytes = send(client_fd, &retry, sizeof(retry), 0);
	ErrorCheck(sent_bytes, "Send Retry");
	return retry;
}

void ErrorCheck(int num, char* str) {
	if (num == -1) {
		perror(str);
		exit(1);
	}
}

void ClientPrintHand() {
	printf("Your Card:");
	for (int i = 1; i < Client_Card.count; i++) {
		switch (Client_Card.suit[i]) {
		case 1:
			printf(" Spade[%d]", Client_Card.number[i]);
			break;
		case 2:
			printf(" Diamond[%d]", Client_Card.number[i]);
			break;
		case 3:
			printf(" Heart[%d]", Client_Card.number[i]);
			break;
		case 4:
			printf(" Clover[%d]", Client_Card.number[i]);
			break;
		default:
			break;
		}
	}
	fflush(stdout);
	printf("\n\n");
}

void ServerPrintHand() {
	printf("Dealer Card:");
	for (int i = 1; i < Server_Card.count; i++) {
		switch (Server_Card.suit[i]) {
		case 1:
			printf(" Spade[%d]", Server_Card.number[i]);
			break;
		case 2:
			printf(" Diamond[%d]", Server_Card.number[i]);
			break;
		case 3:
			printf(" Heart[%d]", Server_Card.number[i]);
			break;
		case 4:
			printf(" Clover[%d]", Server_Card.number[i]);
			break;
		default:
			break;
		}
	}
	fflush(stdout);
	printf("\n\n");
}
