#include "goldchase.h"
#include "Map.h"

int main()
{
  const char *theMine=
    "** ** *****  *****" //18 characters wide
    "** ** ****** *****"
    "**  F *****  *****"
    "** *******  F   **"
    "** *******   *  **"
    " 2  ****  G  F  **"
    "* *****  *  * ****"
    "*  * *  ** 1  ****"
    "*      ***    ****"
    "**********   *****";

  char map[200]; //something arbitrarily large for this test
  const char* ptr=theMine;
  char* mp=map;
  //Convert the ASCII bytes into bit fields drawn from goldchase.h
  while(*ptr!='\0')
  {
    if(*ptr==' ')      *mp=0;
    else if(*ptr=='*') *mp=G_WALL; //A wall
    else if(*ptr=='1') *mp=G_PLR0; //The first player
    else if(*ptr=='2') *mp=G_PLR1; //The second player
    else if(*ptr=='G') *mp=G_GOLD; //Real gold!
    else if(*ptr=='F') *mp=G_FOOL; //Fool's gold
    ++ptr;
    ++mp;
  }
  Map goldMine(map,10,18);
  int a=0;
  goldMine.postNotice("This is a notice");
  while(a=goldMine.getKey()!='Q')
    ;//empty loop
}
