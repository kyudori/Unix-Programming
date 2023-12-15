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
int result;//����� �����ϱ� ���� �����̴�. 

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
	ConnectToServer();//server�� ����� �����ϴ� �Լ��̴�. 

	GameIntro();//���� ���� ȭ���� ����ϴ� �Լ��̴�. 
	SendBalance();//�ʱ� ���� �������� �Է��� server�� �����ϴ� �Լ��̴�. 

	do {//send�� recv�� ������ ���� ����Ǳ� ������ ������ ��� ��쿡 ���� ��� ����ó�����־���. 
		system("clear");
		SendClientBet();//������ ���۵ǰ� ���ñݾ��� �Է¹޾� ������ �����Ѵ�. 
		Client_Card.draw = 1;//�ʱ� ������ ���� draw�� bust�� 1�� �����Ѵ�. 
		Client_Card.bust = 1;
		do {
			ShowInfo();//���� �����ݰ� ���� �ݾ��� ����Ѵ�. 
			if (!Client_Card.draw) break;//client�� ���̻� draw�� ���� �ʰ� ������ break�Ѵ�. 
			ReceiveCard();//server�κ��� ���� ī�带 �����Ѵ�. 
			ClientPrintHand();//client�� �׵��� ���� ī�带 ����Ѵ�. 
			if (!Client_Card.bust) break;//client�� �̹��� ���� ī��� ���� bust�� break�Ѵ�. 
			if (!Client_Card.draw) break;
			SendClientDraw();
		} while (Client_Card.draw);//clientcard.draw�� ���̻� ī�带 �̰� ���� ���������� �ݺ��Ѵ�. 
		ShowInfo();//ȭ���� �ʱ�ȭ�ϰ� ������ ����Ѵ�. 
		ReceiveServerCard();//����� ����ϱ� ���� server ī�带 �����Ѵ�. 
		ClientPrintHand();//client�� server�� �׵��� ���� ī�带 ����Ѵ�. 
		ServerPrintHand();
		ReceiveResult();//����� �����Ѵ�. 
		ReceiveBet();//�ش� ����� ����� balance�� �����Ѵ�. 
	} while (AskForRetry());//client�� �ٸ����� �����Ҳ��� ���� �ش� ���� ���� loop�� Ż������ �ٽ� �������� �����ȴ�. 

	printf("\nFinally, your remaining budget is %d.\n", balance);//������ ���� ���� client�� ���� �������� ����Ѵ�. 

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

void GameIntro() {//ȭ�� �ʱ�ȭ �ϰ� �ʱ� ȯ���޽����� ����Ѵ�. 
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

void SendBalance() {//client���� ���� �������� server�� �����ϰ� balance�� ����Ѵ�. 
	printf("Enter yout budget: ");
	scanf("%d", &balance);
	ssize_t sent_bytes = send(client_fd, &balance, sizeof(balance), 0);
	ErrorCheck(sent_bytes, "Send Balance");
	fflush(stdout);
}

void ShowBalance() {//�Է¹��� �������� ����Ѵ�. ��ܿ� ��ġ�ϰ� ȭ�� �ʱ�ȭ�ϱ� ������ bet�� balance�� �Ѿ���� Ȯ���ϰ� ����ϴ¹��� �� �Լ��� ��ġ�Ͽ���. 
	system("clear");
	printf("******************************************\n");
	printf("*                                        *\n");
	printf("*          Your budget is %d.           *\n", balance);
	printf("*                                        *\n");
	printf("******************************************\n");
	if (bet > balance) printf("Bet amount is over your budget.\n");
}

void ShowInfo() {//���� �����ݰ� ���ñݾ��� ����Ѵ�. 
	system("clear");
	printf("******************************************\n");
	printf("*                                        *\n");
	printf("*          Your budget is %d.           *\n", balance);
	printf("*   Betting amount for this game is %d.  *\n", bet);
	printf("*                                        *\n");
	printf("******************************************\n\n");
}

void SendClientBet() {//client���� server�� ���� �ݾ��� ������ �Լ��� ���� �ݾ��� �������� �Ѿ�� �ٽ� ������� �Ѵ�. 
	int over = 0;
	bet = 0;
	do {
		ShowBalance();
		printf("Enter the amount to bet on this game: ");
		scanf("%d", &bet);//���ñ��� �����Ѵ�. 
		if (bet > balance) over = 1;//���ñ��� �������� �Ѿ�� �ٽ� �Է� �޴´�. 
		else break;
	} while (over);
	ssize_t sent_bytes = send(client_fd, &bet, sizeof(bet), 0);
	ErrorCheck(sent_bytes, "Send Bet");
}

void ReceiveCard() {//server�� ���� ���� ī�� ������ �����Ѵ�.  client���� ī�带 ���� ���� ������ ������ ���鶧 client���� �̰� ��Ű�� ������ ������ �ֱ⶧���� server���� ī�带 �Ѱ� �޴� ������ �����ߴ�. 
	ssize_t received_bytes;
	do {
		received_bytes = recv(client_fd, &Client_Card, sizeof(Client_Card), 0);
	} while (!received_bytes);
}

void ReceiveServerCard() {//���� ������ ����� �˱� ���� server�� card�� �����Ѵ�. 
	ssize_t received_bytes;
	while (1) {
		received_bytes = recv(client_fd, &Server_Card, sizeof(Server_Card), 0);
		if (received_bytes > 0) break;
	}
}

void ReceiveResult() {//����� �����Ͽ� client �¹��п� ���� ����� ����Ѵ�. 
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

void ReceiveBet() {//���� ���� ���� ���п� ���� ������ �������� �����Ѵ�. 
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

int AskForRetry() {//�ٽ� ���� ����� �Լ��� retry������ �����Ͽ� retry���� server�� �����ϰ� retry���� return�Ѵ�. 
	int retry;
	if (balance < 1) {//�������� 0�̸� �Ʒ� ���ڿ��� ����ϰ� 0�� return�Ͽ� ������ �����Ѵ�. 
		printf("Exhausted all the budget you had\n");
		return 0;
	}//�ٽ� ���� ���� ���� �������� ����Ѵ�. 
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
