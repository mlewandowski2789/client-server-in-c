#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

int main() {
  int messageReady;
  struct Message {
    long recipient;
    int messageNumber[3];
    char messageText[1024];
  } testMessage;

  struct Error {
    long recipient;
    int error;
    char errorText[1024];
  } testError;

  int messageQueueId = msgget(128, 0644 | IPC_CREAT);
  int errorQueueId = msgget(256, 0644 | IPC_CREAT);

  int loggedIn = 0;

  int *blockedUsers = malloc(0);
  int numBlockedUsers = 0;
  int *blockedGroups = malloc(0);
  int numBlockedGroups = 0;

  int attempts = 3;

  while (1) {
    messageReady = 0;
    testMessage.recipient = 1;
    testMessage.messageNumber[2] = getpid();
    strcpy(testMessage.messageText, "");
    system("clear");
    printf("What do you want to do?\n");
    if (!loggedIn) {
      printf("0) Log in\n");
      scanf("%d", &testMessage.messageNumber[0]);
      if (testMessage.messageNumber[0] != 0)
        continue;
      printf("Enter password (default: user), remaining tries: %d\n", attempts);
      scanf("%s", testMessage.messageText);
      msgsnd(messageQueueId, &testMessage, sizeof(testMessage), 0);
      msgrcv(errorQueueId, &testError, sizeof(testError), getpid(), 0);
      printf("%s", testError.errorText);
      if (testError.error == 0)
        loggedIn = 1;
      else
	--attempts;
      if(!attempts){
	for(int i = 3; i > 0; --i){
		system("clear");
		printf("No reamining attempts\n");
	  printf("System will self distruct in: %d\n", i);
	  sleep(1);
	}
	system("clear");
	kill(getpid(), SIGKILL);
      }
	
      int response = 0;
      printf("Type 1 to continue\n");
      while (response != 1)
        scanf("%d", &response);
      continue;
    }

    printf("1) Change password\n");
    printf("2) Log out\n");
    printf("3) View list of users\n");
    printf("4) Join a group\n");
    printf("5) Leave a group\n");
    printf("6) View list of groups\n");
    printf("7) Send message to group\n");
    printf("8) Send message to user\n");
    printf("9) Receive message\n");
    printf("10) Block group\n");
    printf("11) Block user\n");
    scanf("%d", &testMessage.messageNumber[0]);

    switch (testMessage.messageNumber[0]) {
    case 1:
      system("clear");
      printf("Enter new password\n");
      scanf("%s", testMessage.messageText);
      messageReady = 1;
      break;
    case 2:
      loggedIn = 0;
      messageReady = 1;
      break;
    case 3:
      system("clear");
      printf("Enter the group number whose list of users you want to view\n");
      printf("If you want to view all users, enter 0\n");
      scanf("%d", &testMessage.messageNumber[1]);
      messageReady = 1;
      break;
    case 4:
      system("clear");
      printf("Enter the group number you want to join\n");
      scanf("%d", &testMessage.messageNumber[1]);
      messageReady = 1;
      break;
    case 5:
      system("clear");
      printf("Enter the group number you want to leave\n");
      scanf("%d", &testMessage.messageNumber[1]);
      messageReady = 1;
      break;
    case 6:
      system("clear");
      messageReady = 1;
      break;
    case 7:
      system("clear");
      printf("Enter the group number you want to send a message to\n");
      scanf("%d", &testMessage.messageNumber[1]);
      printf("Enter your message\n");
      scanf("%s", testMessage.messageText);
      messageReady = 1;
      break;
    case 8:
      system("clear");
      printf("Enter the user number you want to send a message to\n");
      scanf("%d", &testMessage.messageNumber[1]);
      printf("Enter your message\n");
      scanf("%s", testMessage.messageText);
      messageReady = 1;
      break;
    case 9:
      system("clear");
      int messageAccepted = 0;
      while (!messageAccepted) {
        if (msgrcv(messageQueueId, &testMessage, sizeof(testMessage), getpid(),
                   IPC_NOWAIT) == -1) {
          printf("No message\n");
          messageAccepted = 1;
          break;
        }
        messageAccepted = 1;
        for (int i = 0; i < numBlockedGroups; ++i) {
          if (testMessage.messageNumber[1] == blockedGroups[i])
            messageAccepted = 0;
        }
        for (int i = 0; i < numBlockedUsers; ++i) {
          if (testMessage.messageNumber[2] == blockedUsers[i])
            messageAccepted = 0;
        }
        if (messageAccepted)
          printf("Message from %d: %s\n", testMessage.messageNumber[2],
                 testMessage.messageText);
      }
      break;
    case 10:
      system("clear");
      ++numBlockedGroups;
      blockedGroups = realloc(blockedGroups, numBlockedGroups * sizeof(int));
      printf("Enter the group number you want to block\n");
      scanf("%d", &blockedGroups[numBlockedGroups - 1]);
      break;
    case 11:
      system("clear");
      ++numBlockedUsers;
      blockedUsers = realloc(blockedUsers, numBlockedUsers * sizeof(int));
      printf("Enter the user number you want to block\n");
      scanf("%d", &blockedUsers[numBlockedUsers - 1]);
      break;
    default:
      break;
    }
    fflush(stdin);

    if (messageReady) {
      msgsnd(messageQueueId, &testMessage, sizeof(testMessage), 0);
      msgrcv(errorQueueId, &testError, sizeof(testError), getpid(), 0);
      if (testError.error == 0)
        printf("Operation successful\n");
      else
        printf("Operation failed\n");
      printf("%s", testError.errorText);
    }
    int response = 0;
    printf("Type 1 to continue\n");
    while (response != 1)
      scanf("%d", &response);

    fflush(stdin);
  }
  return 1;
}
