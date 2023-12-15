//=============================================
//BLACKJACK
//=============================================

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define CARDNUM 13
#define PORTNUM 9000
#define IPADDR "127.0.0.1"
#define WAITING 3 //wating num
#define HAND 11

//===========================================
//Prototype
//===========================================

void ShowInfo();
void SetServerSocket();
void ErrorCheck(int num, char* str);
void SetBalance(int client_fd);
void GameStart();
void ReceiveBet(int client_fd);
void SendCardClient(int client_fd);
void SendBet(int client_fd);
int ReceiveRetry(int client_fd);
void SetServerDraw();
void ReceiveClientDraw(int client_fd);
void SendResult(int client_fd);
void GameResult();
void Shuffle();
void ServerDraw();
void ClientDraw();
void ServerPrintHand();
void ClientPrintHand();

//===========================================
//Global Variable
//===========================================

int server_fd, client_fd;
int balance;//�������� �����ϴ� �����̴�. 
int bet;//���� ���� ���� �ݾ��� �����ϴ� �����̴�. 
int buff;//������ clientcard.draw�� ���� �����ص� buffer�̴�. 
int win;//2:client win, 1: server win, 0: draw
int Deck_Spade[CARDNUM];//�ߺ��� ã�� ���� �����̴�. ���纰�� ī�� ���� ��ŭ �������. 
int Deck_Clover[CARDNUM];
int Deck_Heart[CARDNUM];
int Deck_Diamond[CARDNUM];
typedef struct {//���� ī����� ������ ��Ƴ��� ����ü�̴�. �ִ�� ������ �ִ� ī��� 11���� 1*4,2*4,3*3�� �� �̻��� �Ѿ�� bust�̴�. 
	int suit[HAND];
	int number[HAND];
	int count;
	int draw;
	int bust;
} Card;
Card Client_Card;
Card Server_Card;

int main(int argc, char* argv[]) {
	system("clear");//ȭ�� �ʱ�ȭ�� 

	SetServerSocket();//��� ���� 
	srand(time(NULL));//���� ������ ���� �ڵ� 
	SetBalance(client_fd);//�������� �Է� �޴� �Լ� 
	do {//���� �ѹ��� ī�带 �޾ƾ� �ϱ� ������ do while�� ��� 
		ReceiveBet(client_fd);//���ñݾ��� �޴� �Լ� 
		ShowInfo();//���� �����ݰ� ���� �ݾ��� ����ϴ� �Լ� 
		GameStart();//�����ݰ� ���� �ݾ��� �������� ���ӽ��� 
	} while (ReceiveRetry(client_fd));//client�� retry �ǻ縦 �����Ͽ� 1,0���� return 1:retry 2:stop 
	printf("Game End.\n");//������ ������ ��� 
	close(server_fd);
	close(client_fd);
}
void SetServerSocket() {
	struct sockaddr_in server_add, client_add;
	socklen_t client_add_size;

	//TCP Socket Creating
	server_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	ErrorCheck(server_fd, "Creating Socket");

	//Struct variable setting
	memset(&client_add, 0, sizeof(client_add));
	memset(&server_add, 0, sizeof(server_add));
	server_add.sin_family = AF_INET;//Socket type
	server_add.sin_port = htons(PORTNUM);//portnumber
	server_add.sin_addr.s_addr = inet_addr(IPADDR);//ipaddress

	ErrorCheck(bind(server_fd, (struct sockaddr*)&server_add, sizeof(server_add)), "Socket Bind");

	ErrorCheck(listen(server_fd, WAITING), "Connection Ready...");

	client_add_size = sizeof(client_add);

	client_fd = accept(server_fd, (struct sockaddr*)&client_add, &client_add_size);
	printf("** Connection Request... <IP> %s || <PORT> %d **", inet_ntoa(client_add.sin_addr), ntohs(client_add.sin_port));
	ErrorCheck(client_fd, "Connection");
}

void ErrorCheck(int num, char* str) {
	if (num == -1) {
		fprintf(stderr, "%s ERROR\n", str);
		exit(1);
	}
	else {
		fprintf(stdout, "%s FINISH\n", str);
	}
}

void SetBalance(int client_fd) {
	int read_size;//�о�� �������� ũ�⸦ �����ϴ� ���� 
	while (1) {//�Է��� ���������� wait
		read_size = recv(client_fd, &balance, sizeof(balance), 0);//�������� client�� ���� ���� 
		if (read_size > 0) break;
	}
	system("clear");
	printf("******************************************\n");
	printf("*                                        *\n");
	printf("*        Player's budget is %d.         *\n", balance);
	printf("*                                        *\n");
	printf("******************************************\n");
	ErrorCheck(read_size, "Data Receive");
	fflush(stdout);//ȭ�� �ʱ�ȭ�� player�� budget ���, ����� �ȵɰ�� fflush�Լ��� ����� stdout ���� ��� 
}

void ShowInfo() {
	system("clear");
	printf("******************************************\n");
	printf("*                                        *\n");
	printf("*        Player's budget is %d.         *\n", balance);
	printf("*   Betting amount for this game is %d.  *\n", bet);
	printf("*                                        *\n");
	printf("******************************************\n\n");
}//���� ������ ���� client�� �����ݰ� ���ñݾ� ��� 

void GameStart() {
	Shuffle();//deck�� Client,Server�� ī�带 �ʱ�ȭ 
	while ((Server_Card.bust && Client_Card.bust) && (Server_Card.draw || Client_Card.draw)) {//���� ���� ������ (client�� draw(�̱�)�� ���ϰų� server�� draw�� ���ϰ�) �׸���(client�� bust�� �ƴϰ� server�� bust�� �ƴϾ���Ѵ�). 
		if (Server_Card.draw) {//server�� draw�� ���ϸ� ���� 
			ServerDraw();//server�� ī�带 �̴´�. servercard�� number�� suit�� �����ϰ� bust���� Ȯ�� 
			ShowInfo();//����� ���� ��� 
			SetServerDraw();//server�� ī�带 �߰��� ������ �����ϴ� �˰����̴�. 
			ServerPrintHand();//server�� �׵��� ���� ī�带 ����Ѵ�. 
		}
		if (!Server_Card.draw) {//server�� draw�� ���� �ʴµ� client�� �߰��� draw�Ұ�� ShowInfo�� �Ҹ��� �ʾ� ȭ���� �ʱ�ȭ ���� �ʾ� ���� ���ǹ��̴�. 
			ShowInfo();//ȭ���� �ʱ�ȭ �ϸ� ��ܿ� ������ ����Ѵ�. 
			ServerPrintHand();//server�� �׵��� ���� ī�带 ����Ѵ�. 
		}
		if (Client_Card.draw || !Server_Card.bust) {//client�� �̱⸦ ���ϰų� server�� bust�� �����Ѵ�. �̴� server�� bust���� �̹� ������ client�� draw�� request �޾��� ���ɼ��� �ֱ⶧���� server bust�϶� ��� ������ �߰��ߴ�. 
			if (buff == 0) break;//���� client�� ������ draw�� ������ �ʴ´ٰ� ���� ������ break�� Ż���ϰ� �ȴ�. 
			ClientDraw();//client�� ī�带 �̴´�. 
			SendCardClient(client_fd);//client�� �׵��� �̾Ҵ� ī�� ������ client�� �����Ѵ�. 
			ClientPrintHand();//client�� �׵��� ���� ī�带 ����Ѵ�. 
			if (!Client_Card.bust) break;//���� client�� bust��� �ٽ� ������ ����� �ʱ� ���� break�Ѵ�. 
			if (!Server_Card.bust) break;//���� server�� bust��� break�Ѵ�. server�� ����Ʈ��� ������ client�� ī�带 �̴´ٰ� ��û�� �������� �ֱ⶧���� �� ��Ȳ�� ó���ϱ� ���� �ۼ��ߴ�. 
			ReceiveClientDraw(client_fd);//client�� �ٽ� �������� ���� �ǻ縦 ���� �޴´�. 
		}
	}
	GameResult();//������ ���и� ����ϰ� server ī�带 client�� �����Ѵ�. client�� �� ������ �˾ƾ� �ϱ� �����̴�. 
	SendResult(client_fd);//����� client�� �����Ѵ�. 
	SendBet(client_fd);//���з� ���� balance ������ client�� �����Ѵ�. 
}

void SetServerDraw() {
	int sum = 0;//���� �����ϱ� ���� �����̴�. 
	for (int i = 0; i < Server_Card.count; i++) {
		sum += Server_Card.number[i];
	}//�׵��� ���� ī���� ���� ���Ѵ�. 
	if (sum > 18) {//���� ���� 18�� �Ѿ�� server�� ���̻� ī�带 ���� �ʴ´�. 
		Server_Card.draw = 0;
	}
}

void ReceiveClientDraw(int client_fd) {//client�� ī�带 �������� ���� �ǻ縦 �����ϴ� �Լ��̴�. 
	ssize_t received_bytes = recv(client_fd, &buff, sizeof(Client_Card.draw), 0);
	Client_Card.draw = buff;//buff�� �����Ͽ� clientcard.draw�� �ʱ� ���� �˼� �ִ�. clientcard.draw�� 0�� �Ȱ� client�� ���̻� �̰� ���� �������� sever�� bust�������� �˼� �ִ�. 
}

void ReceiveBet(int client_fd) {//beting �ݾ��� ���Źް� ����ϴ� �Լ��̴�. 
	int read_size = recv(client_fd, &bet, sizeof(bet), 0);
	printf("******************************************\n");
	printf("*                                        *\n");
	printf("*  Betting amount for this game is %d.  *\n", bet);
	printf("*                                        *\n");
	printf("******************************************\n");
	ErrorCheck(read_size, "Data Receive");
}

int ReceiveRetry(int client_fd) {//������ �ٽ������� ���� ������ �����ϴ� �Լ� �̴�. 
	int retry;
	int read_size = recv(client_fd, &retry, sizeof(retry), 0);
	return retry;
}

void SendCardClient(int client_fd) {//client�� �׵��� ���� ī���� ������ ���� ����ü�� client�� �����ϴ� �Լ��̴�. 
	ssize_t send_bytes = send(client_fd, &Client_Card, sizeof(Client_Card), 0);
}

void SendResult(int client_fd) {//����� ����� client�� �����ϴ� �Լ��̴�. 
	ssize_t send_bytes = send(client_fd, &win, sizeof(win), 0);
}

void SendBet(int client_fd) {//���п� ���� balance ������ ����ϰ� client�� �����ϴ� �Լ��̴�. 
	int after_balance = balance;
	if (win == 2) {
		after_balance += bet;
	}
	else if (win == 1) {
		after_balance -= bet;
	}
	ssize_t send_bytes = send(client_fd, &after_balance, sizeof(win), 0);
	balance = after_balance;//���� draw�ΰ�� balance�� �״�� �ٽ� ����ȴ�. 
}

void GameResult() {//����� ����ϴ� �Լ��̴�. client �¸�:2 client �й�:1 0:���º� 
	int Client_Sum = 0;//client�� �׵��� ���� ī���� ������ �����ϴ� �����̴�. 
	int Server_Sum = 0;//server�� �׵��� ���� ī���� ������ �����ϴ� �����̴�. 
	ssize_t send_bytes = send(client_fd, &Server_Card, sizeof(Server_Card), 0);
	printf("=========================================\n");
	printf("||             Game Result             ||\n");
	if (!Client_Card.bust && !Server_Card.bust) {//client server�� ��� bust�϶� �����̴�. 
		win = 0;
		printf("||                Draw.                ||\n");
		printf("=========================================\n");
		printf("Waiting for player...");
		fflush(stdout);
		return;
	}
	else if (!Client_Card.bust) {//client�� bust�϶� �����̴�. 
		win = 1;
		printf("||             Dealer Win.             ||\n");
		printf("=========================================\n");
		printf("Waiting for player...");
		fflush(stdout);
		return;
	}
	else if (!Server_Card.bust) {//server�� bust�϶� �����̴�. 
		win = 2;
		printf("||             Dealer Lose.            ||\n");
		printf("=========================================\n");
		printf("Waiting for player...");
		fflush(stdout);
		return;
	}//�ƴ϶�� ������ ���� client�� server�� ���Ͽ� ���и� ������. 
	for (int i = 0; i < Client_Card.count; i++) {
		Client_Sum += Client_Card.number[i];
	}
	for (int i = 0; i < Server_Card.count; i++) {
		Server_Sum += Server_Card.number[i];
	}
	if (Client_Sum > Server_Sum) {
		win = 2;
		printf("||             Dealer Lose.            ||\n");
		printf("=========================================\n");
		printf("Waiting for player...\n");
	}
	else if (Client_Sum < Server_Sum) {
		win = 1;
		printf("||             Dealer Win.             ||\n");
		printf("=========================================\n");
		printf("Waiting for player...\n");
	}
	else {
		win = 0;
		printf("||               Draw.                 ||\n");
		printf("=========================================\n");
		printf("Waiting for player...\n");
	}
	fflush(stdout);
}

void Shuffle() {//ī�带 ���� �Լ��� deck,client,server.buff�� ��� �ʱ���·� �����Ѵ�. 
	buff = 1;
	Client_Card.bust = 1;
	Server_Card.bust = 1;
	Client_Card.draw = 1;
	Server_Card.draw = 1;
	Client_Card.count = 1;
	Server_Card.count = 1;
	for (int i = 0; i < 11; i++) {
		Deck_Spade[i] = 0;
		Deck_Clover[i] = 0;
		Deck_Heart[i] = 0;
		Deck_Diamond[i] = 0;
		Client_Card.suit[i] = 0;
		Client_Card.number[i] = 0;
		Server_Card.suit[i] = 0;
		Server_Card.number[i] = 0;
	}
	for (int i = 11; i < 13; i++) {
		Deck_Spade[i] = 0;
		Deck_Clover[i] = 0;
		Deck_Heart[i] = 0;
		Deck_Diamond[i] = 0;
	}
}

void ServerDraw() {
	int draw_num;//���ڿ� �� ������ �����ϴ� �����̴�. 
	int draw_suit;//���翡 �� ������ �����ϴ� �����̴�. 
	int sum = 0;//bust���� ����ϱ� ���� �����̴�. 
	int same = 1;//�ߺ� �������� �˻��ϱ� ���� �����̴�. 
	do {//������ ���� �̰� ���ϰ� �ٽ� ������ �����ؾ��ϹǷ� do while���� ����ߴ�. 
		draw_num = rand() % 13 + 1;//���� ���� ���� �ڵ�, 1~13������ ���� ���� 
		draw_suit = rand() % 4 + 1;//���� ���� ���� �ڵ�, 1~4������ ���� ���� 
		for (int i = 1; i < Server_Card.count; i++) {//�׵��� ���� ī���߿� ���� ī�尡 �ִ��� Ȯ���ϴ� �ڵ��̴�. 
			if (Server_Card.number[i] == draw_num && Server_Card.suit[i] == draw_suit) {//���� servercard.number�� servercard.suit�� ���� ���� ī��� ��� ��ġ�Ҷ� �����̴�. 
				same = 1;//������ �����ϸ� same�� 1�������Ͽ� �ߺ����� ���������� �̴´�. 
				break;//�ش� ī��� �ߺ��̹Ƿ� break�ؼ� ���귮�� �ٿ���. 
			}
			else same = 0;//���� �ߺ��� �ƴ϶�� 0���� ������ �ٸ� ī�尡 �ߺ����� for���� ���� Ȯ���Ѵ�. 
		}
	} while (same && Server_Card.count > 1);//servercard�� count�� 1�̸� �ߺ��� ī�带 ������ �ʾұ� ������ �ѹ� ������ loop�� �������´�. 
	Server_Card.suit[Server_Card.count] = draw_suit;//������ �����Ѵ�. 
	Server_Card.number[Server_Card.count] = draw_num;//���ڸ� �����Ѵ�. 
	Server_Card.count += 1;//ī�� ������ 1�����Ѵ�. 
	for (int i = 1; i < Server_Card.count; i++) {
		sum += Server_Card.number[i];
	}//ī���� ������ ���Ѵ�. 
	if (sum > 21) {//���� ī����� bust�Ǵ��� Ȯ���ϰ� bust��� servercard�� draw�� bust�� 0���� �ʱ�ȭ�ؼ� ��� ���� �˼� �ְ��Ѵ�. 
		Server_Card.draw = 0;
		Server_Card.bust = 0;
		Client_Card.draw = 0;//clientī��� server�� bust�̹Ƿ� �� ���� �ʿ���� �¸��̱� ������ draw�� 0���� �����Ѵ�. ������ server�� Ŭ���̾�Ʈ���� ���� �̱� ������ client�� �̱� ��û�� ���´ٸ� buff�� �����ص� ������ ���Ѵ�. 
	}
}

void ClientDraw() {//client�� ī�带 �̴� �Լ��̴�. 
	int sum = 0;
	int same = 1;
	int draw_num;
	int draw_suit;
	do {
		draw_num = rand() % 13 + 1;
		draw_suit = rand() % 4 + 1;
		for (int i = 1; i < Client_Card.count; i++) {
			if (Client_Card.number[i] == draw_num && Client_Card.suit[i] == draw_suit) {
				same = 1;
				break;
			}
			else same = 0;
		}
	} while (same && Client_Card.count > 1);
	Client_Card.suit[Client_Card.count] = draw_suit;
	Client_Card.number[Client_Card.count] = draw_num;
	Client_Card.count += 1;
	for (int i = 1; i < Client_Card.count; i++) {
		sum += Client_Card.number[i];
	}
	if (sum > 21) {
		Client_Card.bust = 0;//���� client�� bust��� gamestart�Լ��� while�� �ȿ� clientcard.draw���ǹ����� break�� ���� �����Ƿ� servercard.draw�� �������� �ʴ´�. 
	}
}

void ClientPrintHand() {//client�� ���� ī�带 ����ϴ� �Լ��̴�. 
	printf("Player's Card:");
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

void ServerPrintHand() {//server�� ���� ī�带 ����ϴ� �Լ��̴�. 
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
