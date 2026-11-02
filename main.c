#include <stdio.h>
#include <stdlib.h>

#include "deq.h"

int main() {
  //initialize deq
  Deq q=deq_new();
  printf("Initialized DEQ. Length: %d\n", deq_len(q));

  char *s=deq_str(q,0);
  printf("Current contents: %s\n",s);
  free(s);
  
  //Tests for PUT
  printf("\n---Put tests---\n");
  deq_head_put(q, "Dog");
  deq_head_put(q, "Cat");
  deq_tail_put(q, "Bird");
  deq_tail_put(q, "Fish");

  s = deq_str(q,0);
  printf("Expected Result: [Cat Dog Bird Fish]\n");
  printf("Actual Result: [%s]\n", s);
  printf("Length: %d, Expected 4.\n", deq_len(q));
  free(s);

  //Tests for ith
  printf("\n---ith tests---\n");
  printf("Head based ith, index 0, Expected: Cat, Actual: %s\n", (char *)deq_head_ith(q,0));
  printf("Head based ith, index 1, Expected: Dog, Actual: %s\n", (char *)deq_head_ith(q,1));
  printf("Head based ith, index 2, Expected: Bird, Actual: %s\n", (char *)deq_head_ith(q,2));
  printf("Head based ith, index 3, Expected: Fish, Actual: %s\n", (char *)deq_head_ith(q,3));
  printf("Tail based ith, index 0, Expected: Fish, Actual: %s\n", (char *)deq_tail_ith(q,0));
  printf("Tail based ith, index 1, Expected: Bird, Actual: %s\n", (char *)deq_tail_ith(q,1));
  printf("Tail based ith, index 2, Expected: Dog, Actual: %s\n", (char *)deq_tail_ith(q,2));
  printf("Tail based ith, index 3, Expected: Cat, Actual: %s\n", (char *)deq_tail_ith(q,3));
  //Test ith for out of bounds
  Data result = deq_head_ith(q,4);
  printf("Head based ith, index 4 (out of bounds), Expected null, Actual: %s\n", result == NULL?"NULL":(char *)result);
  result = deq_tail_ith(q,4);
  printf("Tail based ith, index 4 (out of bounds), Expected null, Actual: %s\n", result==NULL?"NULL":(char *)result);

  //Tests for REM
  printf("\n---REM tests---\n");
  Data rem_found = deq_head_rem(q, "Cat");
  printf("Removing Cat from head side. Found %s\n", (char *)rem_found);
  rem_found = deq_tail_rem(q,"Fish");
  printf("Removing Fish from tail side. Found %s\n", (char *)rem_found);
  Data rem_not_found = deq_head_rem(q,"Shark");
  printf("Removing nonexistant item from head side. Expected: NULL, Returned: %s\n", (char *)rem_not_found);
  rem_not_found = deq_tail_rem(q,"Whale");
  printf("Removing nonexistant item from tail side. Expected NULL, Returned: %s\n", (char *)rem_not_found);

  //Check Remaining Elements
  s=deq_str(q,0);
  printf("Remaining elements after current tests should be: [Dog Bird]. Actual %s\n", s);

  //Tests for Get
  Data get_head = deq_head_get(q);
  printf("Get expected result: Dog, Actual: %s\n", (char *)get_head);
  Data get_tail = deq_tail_get(q);
  printf("Get expected result: Bird, Actual: %s\n", (char *)get_tail);

  //Tests for empty DEQ
  printf("\n---Empty DEQ Tests---\n");
  printf("Get on head of empty list, Expected: null, Actual: %s\n", (char *)deq_head_get(q));
  printf("Get on tail of empty list, Expected: null, Actual: %s\n", (char *)deq_tail_get(q));
  printf("Head ith on an empty list, Expected: null, Actual: %s\n", (char *)deq_head_ith(q,0));
  printf("Tail ith on an empty list, Expected: null, Actual: %s\n", (char *)deq_tail_ith(q,0));
  free(s);
  deq_del(q,0);
  return 0;
}
