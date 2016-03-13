#include <iostream>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <semaphore.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "goldchase.h"
#include "Map.h"
#include <fstream>

using namespace std;

struct GameBoard
{
   int rows;
   int columns;
   int goldcount;
   int player[5];
   char map[0];
};

int main()
{
   sem_t *mysem;
   int myint;
   GameBoard *goldmap;
   ifstream game_map;
   string mapline, whole_map = "";
   int rows = 0, columns = 0, gold = 0;
   char *ch;
   char player = 'N';
   int current_position;
   bool cleanup = true;


   mysem = sem_open("/ES_semaphore", 
                    O_CREAT|O_EXCL, 
                    S_IRUSR| S_IWUSR| S_IROTH| S_IWOTH| S_IRGRP| S_IWGRP,
                    1);
   if(mysem == SEM_FAILED)
   {
      if(errno == EEXIST)              //If a semaphore of this name doesn't already exist
      {
         perror("Semaphore already exists. Check /dev/shm");
      }
   }

   if(mysem != SEM_FAILED)
   {
      sem_getvalue(mysem, &myint);
      cout << "Semaphore value before taking- "<< myint << endl;

      sem_wait(mysem);                 //Taking the semaphore for first player
      sem_getvalue(mysem, &myint);
      cout << "Semaphore value after taking- "<< myint << endl;

      int shm_fd = shm_open("game_memory", O_CREAT| O_RDWR, S_IRUSR| S_IWUSR);

      game_map.open("mymap.txt");      //Reading in the map
      getline(game_map, mapline);
      gold = stoi(mapline);            //Reading first line to get number of gold
      while(getline(game_map, mapline))
      {
         columns = mapline.length();   //Reading number of columns
         whole_map += mapline;
         rows++;                       //Reading number of rows
      }
      cout << "Rows = " << rows << endl;
      cout << "Coolumns = " << columns << endl;
      ftruncate(shm_fd, rows*columns + sizeof(GameBoard)); //Size of memory
      
      goldmap = (GameBoard*) mmap(NULL, rows*columns, PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0);
                                       //Mmap allocating that much memory to structure
      goldmap->rows = rows;
      goldmap->columns = columns;
      goldmap->goldcount = gold;

      for(int i = 0; i < 5; i++)
      {
         goldmap->player[i] = 0;
      }
      ch = &whole_map[0];

      int index = 0;
      while(*ch != '\0')
      {
			if(*ch == ' ')
			{
				goldmap->map[index] = 0;
			}
         else if(*ch == '*')
         {
            goldmap->map[index] = G_WALL;
         }
         index++;
         ch++;
      }
                         
      //Adding gold
 		int counter = 0;                    //To count till only one gold remains
		index = 0;
		srand(time(NULL));                  //To use random function
		
		while(counter != gold)
		{
			index = rand() % (rows*columns);
			if(goldmap->map[index] == 0)
			{
				if(counter < (gold - 1))      //Counter is not of value 5 out of 6
				{
					goldmap->map[index] = G_FOOL;
				}
				else
				{
					goldmap->map[index] = G_GOLD;
				}
				counter++;
			}
		}

      //Adding player 1
      player = G_PLR0;
      index = 0;
      while(true)
      {
         index = rand() % (rows * columns);
         if(goldmap->map[index] == 0)
         {
            goldmap->map[index] |= player;
            break;
         }
      }
      current_position = index;
      goldmap->player[0] = 1;
      sem_post(mysem);                    //Returning semaphore so others can use
   }

   else
   {
      //For players 2-5
      mysem = sem_open("/ES_semaphore", O_RDWR, S_IRUSR|S_IWUSR, 1);
      sem_wait(mysem);
      int shm_fd = shm_open("game_memory", O_RDWR, S_IRUSR|S_IWUSR);

      read(shm_fd, &rows, sizeof(int));
      read(shm_fd, &columns, sizeof(int));
      goldmap = (GameBoard*) mmap(NULL, rows*columns, PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0);

      for(int i=0; i < 5; i++)
      {
         if(goldmap->player[i] == 0)
         {
            goldmap->player[i] = 1;
            if(i == 0)
            {
               player = G_PLR0;
               break;
            }
            else if(i == 1)
            {
               player = G_PLR1;
               break;
            }
            else if(i == 2)
            {
               player = G_PLR2;
               break;
            }
            else if(i == 3)
            {
               player = G_PLR3;
               break;
            }
            else if(i == 4)
            {
               player = G_PLR4;
               break;
            }
         }
      }
      if(player == 'N')
      {
         cout<<"Only 5 players at a time. Try again after some time."<< endl;
         sem_post(mysem);
         return 0;
      }
      int index = 0;
      while(true)
      {
         index = rand() % (rows * columns);
         if(goldmap->map[index] == 0)
         {
            goldmap->map[index] |= player;
            break;
         }
      }
      current_position = index;
      sem_post(mysem);
   }

   Map goldChase(goldmap->map, goldmap->rows, goldmap->columns);
   goldChase.postNotice("Welcome to Gold Chase!");

   //Moving players
   int key = 0;
   bool realGold = false;
   bool exit = false;
   while(!exit)
   {
		key = goldChase.getKey();
		int current_row = current_position / columns;
		int current_col = current_position % columns;
		switch(key)
		{
         case 104://Left move h 
                  sem_wait(mysem);
                  if(current_col == 0)
                  {
                     if(!realGold)
                     {
								sem_post(mysem);
                        break;
                     }
                     else
                     {
                        goldmap->map[current_position] &= ~player;
                        goldChase.postNotice("YOU WIN!");
                        goldChase.drawMap();
                        exit = true;
								sem_post(mysem);
                        break;
                     }
                  }
                  if(goldmap->map[current_position - 1] == G_WALL)
                  {
							sem_post(mysem);
                     break;
                  }
                  goldmap->map[current_position] &= ~player;
                  current_position = current_position - 1;
                  goldmap->map[current_position] |= player;
                  goldChase.drawMap();
                  if(goldmap->map[current_position] & G_FOOL)
                  {
                     goldChase.postNotice("That's fool's gold! Haha! You lose! Keep looking");
                  }
                  else if(goldmap->map[current_position] & G_GOLD)
                  {
                     goldChase.postNotice("Score! You found the real gold! Goodbye!");
                     realGold = true;
                  }
                  goldChase.drawMap();
                  sem_post(mysem);
         break;

         case 106://Down move j
                  sem_wait(mysem);
                  if((current_position + columns) > rows*columns)
                  {
                      if(!realGold)
                     {
								sem_post(mysem);
                        break;
                     }
                     else
                     {
                        goldmap->map[current_position] &= ~player;
                        goldChase.postNotice("YOU WIN!");
                        goldChase.drawMap();
                        exit = true;
								sem_post(mysem);
                        break;
                     }
                  }
                  if(goldmap->map[current_position + columns] == G_WALL)
                  {
							sem_post(mysem);
                     break;
                  }
                  goldmap->map[current_position] &= ~player;
                  current_position = current_position + columns;
                  goldmap->map[current_position] |= player;
                  goldChase.drawMap();
                  if(goldmap->map[current_position] & G_FOOL)
                  {
                     goldChase.postNotice("That's fool's gold! Haha! You lose! Keep looking");
                  }
                  else if(goldmap->map[current_position] & G_GOLD)
                  {
                     goldChase.postNotice("Score! You found the real gold! Goodbye!");
                     realGold = true;
                  }
                  goldChase.drawMap();
                  sem_post(mysem);
         break;

         case 107://Up move
                  sem_wait(mysem);
                  if((current_position - columns) < 0)
                  {
                      if(!realGold)
                     {
								sem_post(mysem);
                        break;
                     }
                     else
                     {
                        goldmap->map[current_position] &= ~player;
                        goldChase.postNotice("YOU WIN!");
                        goldChase.drawMap();
                        exit = true;
								sem_post(mysem);
                        break;
                     }
                  }
                  if(goldmap->map[current_position - columns] == G_WALL)
                  {
							sem_post(mysem);
                     break;
                  }
                  goldmap->map[current_position] &= ~player;
                  current_position = current_position - columns;
                  goldmap->map[current_position] |= player;
                  goldChase.drawMap(); 
                  if(goldmap->map[current_position] & G_FOOL)
                  {
                     goldChase.postNotice("That's fool's gold! Haha! You lose! Keep looking");
                  }
                  else if(goldmap->map[current_position] & G_GOLD)
                  {
                     goldChase.postNotice("Score! You found the real gold! Goodbye!");
                     realGold = true;
                  }
                  goldChase.drawMap();
                  sem_post(mysem);
         break;

         case 108://Right move
                  sem_wait(mysem);
                  if(current_col == columns-1)
                  {
                      if(!realGold)
                     {
								sem_post(mysem);
                        break;
                     }
                     else
                     {
                        goldmap->map[current_position] &= ~player;
                        goldChase.postNotice("YOU WIN!");
                        goldChase.drawMap();
                        exit = true;
								sem_post(mysem);
                        break;
                     }
                  }
                  if(goldmap->map[current_position + 1] == G_WALL)
                  {
							sem_post(mysem);
                     break;
                  }
                  goldmap->map[current_position] &= ~player;
                  current_position = current_position + 1;
                  goldmap->map[current_position] |= player;
                  goldChase.drawMap();
                  if(goldmap->map[current_position] & G_FOOL)
                  {
                     goldChase.postNotice("That's fool's gold! Haha! You lose! Keep looking");
                  }
                  else if(goldmap->map[current_position] & G_GOLD)
                  {
                     goldChase.postNotice("Score! You found the real gold! Goodbye!");
                     realGold = true;
                  }
                  goldChase.drawMap();
                  sem_post(mysem);
         break;

         case 81:
                 sem_wait(mysem);
                 goldmap->map[current_position] &= ~player;
                 goldChase.drawMap();
                 exit = true;
                 sem_post(mysem);
         break;
     }
   }
   
   //Releasing players
   sem_wait(mysem);
   if(player == G_PLR0)
   {
      goldmap->player[0] = 0;
   }
   else if(player == G_PLR1)
   {
      goldmap->player[1] = 0;
   }
   else if(player == G_PLR2)
   {
      goldmap->player[2] = 0;
   }
   else if(player == G_PLR3)
   {
      goldmap->player[3] = 0;
   }
   else if(player == G_PLR4)
   {
      goldmap->player[4] = 0;
   }
   sem_post(mysem);

   //If all players have exited then exit game
   sem_wait(mysem);
   for(int i = 0; i < 5; i++)
   {
      if(goldmap->player[i] == 0)
      {
      
      }
      else
      {
         cleanup = false;         
         break;
      }
   }
   sem_post(mysem);
   
   if(cleanup)
   {
      shm_unlink("game_memory");
		sem_close(mysem);
		sem_unlink("/ES_semaphore");
   }
   return 0;
}
