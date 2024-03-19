#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>

struct Group {
  int *userIds;
  int numUsers;
  int maxUsers;
};

struct GroupList {
  struct Group *groups;
  int numGroups;
  int maxGroups;
};

struct User {
  int id;
  char password[16];
};

void addGroup(struct GroupList *list) {
  if (list->numGroups == list->maxGroups) {
    list->maxGroups += 1;
    list->groups =
        realloc(list->groups, list->maxGroups * sizeof(struct Group));
  }
  struct Group newGroup = {
      .userIds = malloc(sizeof(int)), .numUsers = 0, .maxUsers = 1};
  list->groups[list->numGroups++] = newGroup;
}

int addUserToGroup(struct GroupList *list, int groupIndex, int userId) {
  struct Group *group = &list->groups[groupIndex];

  for (int i = 0; i < group->numUsers; ++i)
    if (group->userIds[i] == userId)
      return 1;

  if (group->numUsers == group->maxUsers) {
    group->maxUsers += 1;
    group->userIds = realloc(group->userIds, group->maxUsers * sizeof(int));
  }
  group->userIds[group->numUsers++] = userId;
  return 0;
}

int removeUserFromGroup(struct GroupList *list, int groupIndex, int userId) {
  struct Group *group = &list->groups[groupIndex];

  int found = 0;
  for (int i = 0; i < group->numUsers; ++i)
    if (group->userIds[i] == userId)
      found = 1;

  if (!found)
    return 1;

  int newNumUsers = 0;
  for (int i = 0; i < group->numUsers; i++) {
    if (group->userIds[i] != userId) {
      group->userIds[newNumUsers++] = group->userIds[i];
    }
  }
  group->numUsers = newNumUsers;
  return 0;
}

char *printUsersInGroup(struct GroupList *list, int groupIndex) {
  struct Group *group = &list->groups[groupIndex];
  char *userIds = malloc(1024);
  strcpy(userIds, "Selected user IDs: ");

  for (int i = 0; i < group->numUsers; i++) {
    char userIdStr[8];
    snprintf(userIdStr, sizeof(userIdStr), "%d ", group->userIds[i]);
    strcat(userIds, userIdStr);
  }
  strcat(userIds, "\n");
  return userIds;
}

char *printGroups(struct GroupList *list) {
  char *groups = malloc(1024);
  strcpy(groups, "List of groups: ");

  for (int i = 1; i < list->numGroups; i++) {
    if (list->groups[i].numUsers > 0) {
      char groupsStr[8];
      snprintf(groupsStr, sizeof(groupsStr), "%d ", i);
      strcat(groups, groupsStr);
    }
  }
  strcat(groups, "\n");
  return groups;
}

int main() {

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

  struct User *users;
  users = malloc(0);
  int userCount = 0;

  struct GroupList list;
  list.groups = malloc(sizeof(struct Group));
  list.numGroups = 0;
  list.maxGroups = 1;
  addGroup(&list);

  while (1) {
    strcpy(testError.errorText, "");
    testError.error = 0;
    msgrcv(messageQueueId, &testMessage, sizeof(testMessage), 1, 0);
    testError.recipient = testMessage.messageNumber[2];
    switch (testMessage.messageNumber[0]) {
    case 0:
      printf("login\n");
      int userFound = 0;
      for (int i = 0; i < userCount; ++i) {
        if (testMessage.messageNumber[2] == users[i].id) {
          userFound = 1;
          if (!strcmp(testMessage.messageText, users[i].password)) {
            testError.error =
                addUserToGroup(&list, 0, testMessage.messageNumber[2]);
            strcpy(testError.errorText, "Correct password\n");
          } else {
            testError.error = 1;
            strcpy(testError.errorText, "Wrong password\n");
          }
        }
      }
      if (!userFound) {
        ++userCount;
        users = realloc(users, userCount * sizeof(struct User));
        users[userCount - 1].id = testMessage.messageNumber[2];
        strcpy(users[userCount - 1].password, "user");
        if (!strcmp(testMessage.messageText, users[userCount - 1].password)) {
          testError.error =
              addUserToGroup(&list, 0, testMessage.messageNumber[2]);
          strcpy(testError.errorText, "Correct password\n");
        } else {
          testError.error = 1;
          strcpy(testError.errorText, "Wrong password\n");
        }
      }
      break;

    case 1:
      printf("password\n");
      for (int i = 0; i < userCount; ++i) {
        if (testMessage.messageNumber[2] == users[i].id) {
          strcpy(users[i].password, testMessage.messageText);
        }
      }
      break;

    case 2:
      printf("logout\n");
      testError.error =
          removeUserFromGroup(&list, 0, testMessage.messageNumber[2]);
      break;

    case 3:
      printf("view members\n");
      if (testMessage.messageNumber[1] > list.numGroups) {
        testError.error = 1;
        break;
      }
      char *users = printUsersInGroup(&list, testMessage.messageNumber[1]);
      strcpy(testError.errorText, users);
      break;

    case 4:
      printf("join group\n");
      while (testMessage.messageNumber[1] >= list.numGroups)
        addGroup(&list);
      testError.error = addUserToGroup(&list, testMessage.messageNumber[1],
                                       testMessage.messageNumber[2]);
      break;

    case 5:
      printf("leave group\n");
      if (testMessage.messageNumber[1] > list.numGroups)
        testError.error = 1;
      else
        testError.error = removeUserFromGroup(
            &list, testMessage.messageNumber[1], testMessage.messageNumber[2]);
      break;

    case 6:
      printf("view groups\n");
      char *groups = printGroups(&list);
      strcpy(testError.errorText, groups);
      break;
    case 7:
      printf("message group\n");
      if (testMessage.messageNumber[1] > list.numGroups) {
        testError.error = 1;
        break;
      }
      if (list.groups[testMessage.messageNumber[1]].numUsers == 0) {
        testError.error = 1;
        break;
      }
      for (int i = 0; i < list.groups[testMessage.messageNumber[1]].numUsers;
           ++i) {
        testMessage.recipient =
            list.groups[testMessage.messageNumber[1]].userIds[i];
        msgsnd(messageQueueId, &testMessage, sizeof(testMessage), 0);
      }
      break;
    case 8:
      printf("message user\n");
      int found = 0;
      for (int i = 0; i < list.groups[0].numUsers; ++i) {
        if (list.groups[0].userIds[i] == testMessage.messageNumber[1]) {
          found = 1;
          testMessage.recipient = testMessage.messageNumber[1];
          testMessage.messageNumber[1] = 0;
          msgsnd(messageQueueId, &testMessage, sizeof(testMessage), 0);
          break;
        }
      }
      if (!found)
        testError.error = 1;
      break;
    default:
      break;
    }
    msgsnd(errorQueueId, &testError, sizeof(testError), 0);
  }
  return 0;
}
