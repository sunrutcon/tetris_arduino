
// Code Example for jolliFactory 2X Bi-color LED Matrices Tetris Game example 1.1


/* Wirings

SPI connections between Arduino Nano/UNO and the 2X Bi-color LED Matrix Driver Modules are MOSI (Pin 11), SCK (Pin 13) and SS (Pin 10) at the Arduino side and Din, CLK and Load pins at the LED Matrix Driver Module respectively.        
8 ohms 0.5W speaker is connected via a 100 ohms resistor to Arduino Digital Pin 9 and Gnd
4 pushbutton switches are connected for game control:
Pin 4 - Rotate
Pin 5 - Right
Pin 6 - Left
Pin 7 - Down


Please visit instructable at http://www.instructables.com/id/Arduino-based-Bi-color-LED-Matrix-Tetris-Game/ for more detail

*/



/* Tetris Code

Adapted from Tetris.ino code by 

  Jae Yeong Bae
  UBC ECE
  jocker.tistory.com

*/



/* ============================= LED Matrix Display ============================= */
#include <SPI.h>          
       
#define GREEN 0                          
#define RED 1                            

#define offREDoffGREEN 0
#define offREDonGREEN 1
#define onREDoffGREEN 2

#define ISR_FREQ 190     //190=650Hz    // Sets the speed of the ISR

int SPI_CS = 10;// This SPI Chip Select pin controls the MAX7219
int bi_maxInUse = 2; //No. of Bi-color LED Matrix used
int maxInShutdown = RED; // indicates which LED Matrix color is currently off
int SetbrightnessValue = 15;
int colorMode = '3';  // default color (1 = RED, 2 = GREEN, 3 = ORANGE, 4 = blank off)




/* ============================= Audio ============================= */
int speakerOut = 9;

#define mC 1911
#define mC1 1804
#define mD 1703
#define mEb 1607
#define mE 1517
#define mF 1432
#define mF1 1352
#define mG 1276
#define mAb 1204
#define mA 1136
#define mBb 1073
#define mB 1012
#define mc 955
#define mc1 902
#define md 851
#define meb 803
#define me 758
#define mf 716
#define mf1 676
#define mg 638
#define mab 602
#define ma 568
#define mbb 536
#define mb 506

#define mp 0  //pause


// MELODY and TIMING  =======================================
//  melody[] is an array of notes, accompanied by beats[],
//  which sets each note's relative length (higher #, longer note)
int melody[] = {mg};

// note durations: 4 = quarter note, 8 = eighth note, etc.:
int beats[]  = {8};

//int MAX_COUNT = sizeof(melody) / 2; // Melody length, for looping.
int MAX_COUNT = sizeof(melody) / sizeof(int); // Melody length, for looping.

// Set overall tempo
long tempo = 20000;
// Set length of pause between notes
int pause = 1000;
// Loop variable to increase Rest length
int rest_count = 100; //<-BLETCHEROUS HACK; See NOTES

// Initialize core variables
int tone_ = 0;
int beat = 0;
long duration  = 0;




/* ========================== Tetris Game ========================= */
long delays = 0;
short delay_ = 500;
long bdelay = 0;
short buttondelay = 150;
short btdowndelay = 30;
short btsidedelay = 80;
unsigned char blocktype;
unsigned char blockrotation;

boolean block[8][18]; //2 extra for rotation
boolean pile[8][16];
boolean disp[8][16];

boolean gameoverFlag = false;
boolean selectColor = RED;

unsigned long startTime;
unsigned long elapsedTime;
int cnt = 0;

int buttonRotate = 4; // Rotate
int buttonRight = 5;  // Right
int buttonLeft = 6;   // Left
int buttonDown = 7;   // Down


//**********************************************************************************************************************************************************  
void setup() 
{
  pinMode(SPI_CS, OUTPUT);
  pinMode(speakerOut, OUTPUT); 
  
  TriggerSound();

  Serial.begin (9600);
  Serial.println("jolliFactory Tetris LED Matrix Game example 1.1");              

  SPI.begin(); //setup SPI Interface

  bi_maxTransferAll(0x0F, 0x00);   // 00 - Turn off Test mode
  bi_maxTransferAll(0x09, 0x00);   //Register 09 - BCD Decoding  // 0 = No decoding
  bi_maxTransferAll(0x0B, 0x07);   //Register B - Scan limit 1-7  // 7 = All LEDS
  bi_maxTransferAll(0x0C, 0x01);   // 01 = on 00 = Power saving mode or shutdown

  setBrightness();

  setISRtimer();                        // setup the timer
  startISR();                           // start the timer to toggle shutdown

  clearDisplay(GREEN);
  clearDisplay(RED);

  
  int seed = 
  (analogRead(0)+1)*
  (analogRead(1)+1)*
  (analogRead(2)+1)*
  (analogRead(3)+1);
  randomSeed(seed);
  random(10,9610806);
  seed = seed *random(3336,15679912)+analogRead(random(4)) ;
  randomSeed(seed);  
  random(10,98046);

  
  cli();//stop interrupts

//set timer0 interrupt at 2kHz
  TCCR1A = 0;// set entire TCCR0A register to 0
  TCCR1B = 0;// same for TCCR0B
  TCNT1  = 0;//initialize counter value to 0
  // set compare match register for 2khz increments
  OCR1A = 259;// = (16*10^6) / (2000*64) - 1 (must be <256)
  // turn on CTC mode
  TCCR1A |= (1 << WGM01);
  // Set CS11 and CS10 bits for 1024 prescaler
  TCCR1B |= (1 << CS12) | (1 << CS10);   
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE0A);

  sei();//allow interrupts  
  

  pinMode(buttonRotate, INPUT_PULLUP); // Rotate
  pinMode(buttonRight, INPUT_PULLUP);  // Right
  pinMode(buttonLeft, INPUT_PULLUP);   // Left
  pinMode(buttonDown, INPUT_PULLUP);   // Down
    
  newBlock();
  updateLED();    
}




//**********************************************************************************************************************************************************  
void loop() 
{
  delay(30);
  
  if (delays < millis())
   {
     delays = millis() + delay_;
     movedown();
   }       
    
   //buttun actions
  int button = readBut();
      
  if (button == 1) //up=rotate
    rotate();
  if (button == 2) //right=moveright
    moveright();    
  if (button == 3) //left=moveleft
    moveleft();
  if (button == 4) //down=movedown
    movedown();  
}



//**********************************************************************************************************************************************************  
boolean moveleft()
{  
  TriggerSound();
  
  if (space_left())
  {
    int i;
    int j;
    for (i=0;i<7;i++)
    {
      for (j=0;j<16;j++)      
      {
        block[i][j]=block[i+1][j];
      }
    }
    
    for (j=0;j<16;j++)      
    {
      block[7][j]=0;
    }    

    updateLED();
    return 1;
  }

  return 0;
}



//**********************************************************************************************************************************************************  
boolean moveright()
{
  TriggerSound();
  
  if (space_right())
  {
    int i;
    int j;
    for (i=7;i>0;i--)
    {
      for (j=0;j<16;j++)      
      {
        block[i][j]=block[i-1][j];
      }
    }

    for (j=0;j<16;j++)      
    {
      block[0][j]=0;
    }    
    
   updateLED(); 
   return 1;   
  
  }
  return 0;
}



//**********************************************************************************************************************************************************  
int readBut()
{
  if (bdelay > millis())
  {
    return 0;
  }
  if ((digitalRead(buttonLeft) == LOW))
  {
    //left
    bdelay = millis() + btsidedelay;    
    return 2;
  }
  
  if ((digitalRead(buttonDown) == LOW))
  {
    //down
    bdelay = millis() + btdowndelay;    
    return 4;
  }    
  if ((digitalRead(buttonRight) == LOW))
  {
    //right
    bdelay = millis() + btsidedelay;
    return 3;
  }  
  if ((digitalRead(buttonRotate) == LOW))
  {
    //rotate
    bdelay = millis() + buttondelay;
    return 1;
  }  
  
  return 0;
}



//**********************************************************************************************************************************************************  
void updateLED()
{
  int i;
  int j;  
  for (i=0;i<8;i++)
  {
    for (j=0;j<16;j++)
    {
      disp[i][j] = block[i][j] | pile[i][j];
    }
  }
}



//**********************************************************************************************************************************************************  
void rotate()
{
  TriggerSound();
  
  //skip for square block(3)
  if (blocktype == 3) return;
  
  int xi;
  int yi;
  int i;
  int j;
  //detect left
  for (i=7;i>=0;i--)
  {
    for (j=0;j<16;j++)
    {
      if (block[i][j])
      {
        xi = i;
      }
    }
  }
  
  //detect up
  for (i=15;i>=0;i--)
  {
    for (j=0;j<8;j++)
    {
      if (block[j][i])
      {
        yi = i;
      }
    }
  }  
    
  if (blocktype == 0)
  {
    if (blockrotation == 0) 
    {
      
      
      if (!space_left())
      {
        if (space_right3())
        {
          if (!moveright())
            return;
          xi++;
        }
        else return;
      }     
      else if (!space_right())
      {
        if (space_left3())
        {
          if (!moveleft())
            return;
          if (!moveleft())
            return;          
          xi--;
          xi--;        
        }
        else
          return;
      }
      else if (!space_right2())
      {
        if (space_left2())
        {
          if (!moveleft())
            return;          
          xi--;      
        }
        else
          return;
      }   
   
      
      block[xi][yi]=0;
      block[xi][yi+2]=0;
      block[xi][yi+3]=0;      
      
      block[xi-1][yi+1]=1;
      block[xi+1][yi+1]=1;
      block[xi+2][yi+1]=1;      

      blockrotation = 1;
    }
    else
    {
      block[xi][yi]=0;
      block[xi+2][yi]=0;
      block[xi+3][yi]=0;
      
      block[xi+1][yi-1]=1;
      block[xi+1][yi+1]=1;
      block[xi+1][yi+2]=1;

      blockrotation = 0;
    }    
  }
  
  //offset to mid
  xi ++;  
  yi ++;  
  
  if (blocktype == 1)
  {
    if (blockrotation == 0)
    {
      block[xi-1][yi-1] = 0;
      block[xi-1][yi] = 0;
      block[xi+1][yi] = 0;

      block[xi][yi-1] = 1;
      block[xi+1][yi-1] = 1;
      block[xi][yi+1] = 1;      
      
      blockrotation = 1;
    }
    else if (blockrotation == 1)
    {
      if (!space_left())
      {
        if (!moveright())
          return;
        xi++;
      }        
      xi--;
      
      block[xi][yi-1] = 0;
      block[xi+1][yi-1] = 0;
      block[xi][yi+1] = 0;      
      
      block[xi-1][yi] = 1;
      block[xi+1][yi] = 1;
      block[xi+1][yi+1] = 1;      
      
      blockrotation = 2;      
    }
    else if (blockrotation == 2)
    {
      yi --;
      
      block[xi-1][yi] = 0;
      block[xi+1][yi] = 0;
      block[xi+1][yi+1] = 0;      
      
      block[xi][yi-1] = 1;
      block[xi][yi+1] = 1;
      block[xi-1][yi+1] = 1;      
      
      blockrotation = 3;            
    }
    else
    {
      if (!space_right())
      {
        if (!moveleft())
          return;
        xi--;
      }
      block[xi][yi-1] = 0;
      block[xi][yi+1] = 0;
      block[xi-1][yi+1] = 0;        

      block[xi-1][yi-1] = 1;
      block[xi-1][yi] = 1;
      block[xi+1][yi] = 1;
      
      blockrotation = 0;          
    }  
  }



  if (blocktype == 2)
  {
    if (blockrotation == 0)
    {
      block[xi+1][yi-1] = 0;
      block[xi-1][yi] = 0;
      block[xi+1][yi] = 0;

      block[xi][yi-1] = 1;
      block[xi+1][yi+1] = 1;
      block[xi][yi+1] = 1;      
      
      blockrotation = 1;
    }
    else if (blockrotation == 1)
    {
      if (!space_left())
      {
        if (!moveright())
          return;
        xi++;
      }              
      xi--;
      
      block[xi][yi-1] = 0;
      block[xi+1][yi+1] = 0;
      block[xi][yi+1] = 0;      
      
      block[xi-1][yi] = 1;
      block[xi+1][yi] = 1;
      block[xi-1][yi+1] = 1;      
      
      blockrotation = 2;      
    }
    else if (blockrotation == 2)
    {
      yi --;
      
      block[xi-1][yi] = 0;
      block[xi+1][yi] = 0;
      block[xi-1][yi+1] = 0;      
      
      block[xi][yi-1] = 1;
      block[xi][yi+1] = 1;
      block[xi-1][yi-1] = 1;      
      
      blockrotation = 3;            
    }
    else
    {
      if (!space_right())
      {
        if (!moveleft())
          return;
        xi--;
      }      
      block[xi][yi-1] = 0;
      block[xi][yi+1] = 0;
      block[xi-1][yi-1] = 0;        

      block[xi+1][yi-1] = 1;
      block[xi-1][yi] = 1;
      block[xi+1][yi] = 1;
      
      blockrotation = 0;          
    }  
  }
  
  if (blocktype == 4)
  {
    if (blockrotation == 0)
    {
      block[xi+1][yi-1] = 0;
      block[xi-1][yi] = 0;

      block[xi+1][yi] = 1;
      block[xi+1][yi+1] = 1;      
      
      blockrotation = 1;
    }
    else
    {
      if (!space_left())
      {
        if (!moveright())
          return;
        xi++;
      }              
      xi--;
      
      block[xi+1][yi] = 0;
      block[xi+1][yi+1] = 0;      
      
      block[xi-1][yi] = 1;
      block[xi+1][yi-1] = 1;
      
      blockrotation = 0;          
    }  
  }  


  if (blocktype == 5)
  {
    if (blockrotation == 0)
    {
      block[xi][yi-1] = 0;
      block[xi-1][yi] = 0;
      block[xi+1][yi] = 0;

      block[xi][yi-1] = 1;
      block[xi+1][yi] = 1;
      block[xi][yi+1] = 1;      
      
      blockrotation = 1;
    }
    else if (blockrotation == 1)
    {
      if (!space_left())
      {
        if (!moveright())
          return;
        xi++;
      }              
      xi--;
      
      block[xi][yi-1] = 0;
      block[xi+1][yi] = 0;
      block[xi][yi+1] = 0;
      
      block[xi-1][yi] = 1;
      block[xi+1][yi] = 1;
      block[xi][yi+1] = 1;
      
      blockrotation = 2;      
    }
    else if (blockrotation == 2)
    {
      yi --;
      
      block[xi-1][yi] = 0;
      block[xi+1][yi] = 0;
      block[xi][yi+1] = 0;     
      
      block[xi][yi-1] = 1;
      block[xi-1][yi] = 1;
      block[xi][yi+1] = 1;      
      
      blockrotation = 3;            
    }
    else
    {
      if (!space_right())
      {
        if (!moveleft())
          return;
        xi--;
      }      
      block[xi][yi-1] = 0;
      block[xi-1][yi] = 0;
      block[xi][yi+1] = 0;      
      
      block[xi][yi-1] = 1;
      block[xi-1][yi] = 1;
      block[xi+1][yi] = 1;
      
      blockrotation = 0;          
    }  
  }
  
  if (blocktype == 6)
  {
    if (blockrotation == 0)
    {
      block[xi-1][yi-1] = 0;
      block[xi][yi-1] = 0;

      block[xi+1][yi-1] = 1;
      block[xi][yi+1] = 1;      
      
      blockrotation = 1;
    }
    else
    {
      if (!space_left())
      {
        if (!moveright())
          return;
        xi++;
      }              
      xi--;
      
      block[xi+1][yi-1] = 0;
      block[xi][yi+1] = 0;      
      
      block[xi-1][yi-1] = 1;
      block[xi][yi-1] = 1;
      
      blockrotation = 0;          
    }  
  }  

  //if rotating made block and pile overlap, push rows up
  while (!check_overlap())
  {
    for (i=0;i<18;i++)
    {
      for (j=0;j<8;j++)
      {
         block[j][i] = block[j][i+1];
      }
    }
    delays = millis() + delay_;
  }
  
  
  updateLED();    
}



//**********************************************************************************************************************************************************  
void movedown()
{ 
  if (space_below())
  {
    //move down
    int i;
    for (i=15;i>=0;i--)
    {
      int j;
      for (j=0;j<8;j++)
      {
        block[j][i] = block[j][i-1];
      }
    }
    for (i=0;i<7;i++)
    {
      block[i][0] = 0;
    }
  }
  else
  {
    //merge and new block
    int i;
    int j;    
    for (i=0;i<8;i++)
    {
     for(j=0;j<16;j++)
     {
       if (block[i][j])
       {
         pile[i][j]=1;
         block[i][j]=0;
       }
     }
    }
    newBlock();   
  }
  updateLED();  
}



//**********************************************************************************************************************************************************  
boolean check_overlap()
{
  int i;
  int j;  
  for (i=0;i<16;i++)
  {
    for (j=0;j<7;j++)
    {
       if (block[j][i])
       {
         if (pile[j][i])
           return false;
       }        
    }
  }
  for (i=16;i<18;i++)
  {
    for (j=0;j<7;j++)
    {
       if (block[j][i])
       {
         return false;
       }        
    }
  }  
  return true;
}



//**********************************************************************************************************************************************************  
void check_gameover()
{
  int i;
  int j;
  int cnt=0;;
  
  for(i=15;i>=0;i--)
  {
    cnt=0;
    for (j=0;j<8;j++)
    {
      if (pile[j][i])
      {
        cnt ++;
      }
    }    
    if (cnt == 8)
    {
      for (j=0;j<8;j++)
      {
        pile[j][i]=0;
      }        
      updateLED();
      delay(50);
      
      int k;
      for(k=i;k>0;k--)
      {
        for (j=0;j<8;j++)
        {
          pile[j][k] = pile[j][k-1];
        }                
      }
      for (j=0;j<8;j++)
      {
        pile[j][0] = 0;
      }        
      updateLED();      
      delay(50);      
      i++;     
      
      
    
    }
  }  
  
  
  for(i=0;i<8;i++)
  {
    if (pile[i][0])
      gameover();
  }
  return;
}



//**********************************************************************************************************************************************************  
void gameover()
{
  int i;
  int j;
  

  gameoverFlag = true;
  startTime = millis();       
       
  delay(300);       
            
  while(true)      //To re-play if any buttons depressed again
  {      
    int button = readBut();
    
    if ((button < 5) && (button > 0))
    {
      gameoverFlag = false;    
    
      for(i=15;i>=0;i--)
      {
        for (j=0;j<8;j++)
        {
          pile[j][i]=0;
        }             
      }
    
      break;
    }  
  }  
}



//**********************************************************************************************************************************************************  
void newBlock()
{
  check_gameover();
  
  if (selectColor == RED)
    selectColor = GREEN;
  else
    selectColor = RED;

  
  blocktype = random(7);

  
  if (blocktype == 0)
  // 0
  // 0
  // 0
  // 0
  {
    block[3][0]=1;
    block[3][1]=1;
    block[3][2]=1;
    block[3][3]=1;      
  }

  if (blocktype == 1)
  // 0
  // 0 0 0
  {
    block[2][0]=1;
    block[2][1]=1;
    block[3][1]=1;
    block[4][1]=1;        
  }
  
  if (blocktype == 2)
  //     0
  // 0 0 0
  {
    block[4][0]=1;
    block[2][1]=1;
    block[3][1]=1;
    block[4][1]=1;         
  }

  if (blocktype == 3)
  // 0 0
  // 0 0
  {
    block[3][0]=1;
    block[3][1]=1;
    block[4][0]=1;
    block[4][1]=1;          
  }    

  if (blocktype == 4)
  //   0 0
  // 0 0
  {
    block[4][0]=1;
    block[5][0]=1;
    block[3][1]=1;
    block[4][1]=1;         
  }    
  
  if (blocktype == 5)
  //   0
  // 0 0 0
  {
    block[4][0]=1;
    block[3][1]=1;
    block[4][1]=1;
    block[5][1]=1;       
  }        

  if (blocktype == 6)
  // 0 0
  //   0 0
  {
    block[3][0]=1;
    block[4][0]=1;
    block[4][1]=1;
    block[5][1]=1;         
  }    

  blockrotation = 0;
}



//**********************************************************************************************************************************************************  
boolean space_below()
{ 
  int i;
  int j;  
  for (i=15;i>=0;i--)
  {
    for (j=0;j<8;j++)
    {
       if (block[j][i])
       {
         if (i == 15)
           return false;
         if (pile[j][i+1])
         {
           return false;
         }      
       }        
    }
  }
  return true;
}



//**********************************************************************************************************************************************************  
boolean space_left2()
{ 
  int i;
  int j;  
  for (i=15;i>=0;i--)
  {
    for (j=0;j<8;j++)
    {
       if (block[j][i])
       {
         if (j == 0 || j == 1)
           return false;
         if (pile[j-1][i] | pile[j-2][i])
         {
           return false;
         }      
       }        
    }
  }
  return true;
}



//**********************************************************************************************************************************************************  
boolean space_left3()
{ 
  int i;
  int j;  
  for (i=15;i>=0;i--)
  {
    for (j=0;j<8;j++)
    {
       if (block[j][i])
       {
         if (j == 0 || j == 1 ||j == 2 )
           return false;
         if (pile[j-1][i] | pile[j-2][i]|pile[j-3][i])
         {
           return false;
         }      
       }        
    }
  }
  return true;
}



//**********************************************************************************************************************************************************  
boolean space_left()
{ 
  int i;
  int j;  
  for (i=15;i>=0;i--)
  {
    for (j=0;j<8;j++)
    {
       if (block[j][i])
       {
         if (j == 0)
           return false;
         if (pile[j-1][i])
         {
           return false;
         }      
       }        
    }
  }
  return true;
}



//**********************************************************************************************************************************************************  
boolean space_right()
{ 
  int i;
  int j;  
  for (i=15;i>=0;i--)
  {
    for (j=0;j<8;j++)
    {
       if (block[j][i])
       {
         if (j == 7)
           return false;
         if (pile[j+1][i])
         {
           return false;
         }      
       }        
    }
  }
  return true;
}



//**********************************************************************************************************************************************************  
boolean space_right3()
{ 
  int i;
  int j;  
  for (i=15;i>=0;i--)
  {
    for (j=0;j<8;j++)
    {
       if (block[j][i])
       {
         if (j == 7||j == 6||j == 5)
           return false;
         if (pile[j+1][i] |pile[j+2][i] | pile[j+3][i])
         {
           return false;
         }      
       }        
    }
  }
  return true;
}



//**********************************************************************************************************************************************************  
boolean space_right2()
{ 
  int i;
  int j;  
  for (i=15;i>=0;i--)
  {
    for (j=0;j<8;j++)
    {
       if (block[j][i])
       {
         if (j == 7 || j == 6)
           return false;
         if (pile[j+1][i] |pile[j+2][i])
         {
           return false;
         }      
       }        
    }
  }
  return true;
}



//**********************************************************************************************************************************************************  
ISR(TIMER1_COMPA_vect){  //change the 0 to 1 for timer1 and 2 for timer2
    LEDRefresh();
}



//**********************************************************************************************************************************************************  
void LEDRefresh()
{
    int i;
    int k;

    boolean tmpdispUpper[8][8];
    boolean tmpdispLower[8][8];
     
    boolean tmppileUpper[8][8];
    boolean tmppileLower[8][8];
     
  
    //rotate 90 degrees for upper Bicolor LED matrix
    for (k=0;k<8;k++)
    {
      for(i=0;i<8;i++)
      {
        tmpdispUpper[k][i]=disp[i][k];
      }
    }  
  
  
    //rotate 90 degrees for lower Bicolor LED matrix
    for (k=8;k<16;k++)
    {
      for(i=0;i<8;i++)
      {
        tmpdispLower[k-8][i]=disp[i][k];
      }
    }  



    //For pile
    //rotate 90 degrees for upper Bicolor LED matrix
    for (k=0;k<8;k++)
    {
      for(i=0;i<8;i++)
      {
        tmppileUpper[k][i]=pile[i][k];
      }
    }  
  
  
    //rotate 90 degrees for lower Bicolor LED matrix
    for (k=8;k<16;k++)
    {
      for(i=0;i<8;i++)
      {
        tmppileLower[k-8][i]=pile[i][k];
      }
    }  

  

    for(i=0;i<8;i++)
    {      
       byte upper = 0;
       int b;
       for(b = 0;b<8;b++)
       {
         upper <<= 1;
         if (tmpdispUpper[b][i]) upper |= 1;
       }
       
       
       byte lower = 0;
       for(b = 0;b<8;b++)
       {
         lower <<= 1;
         if (tmpdispLower[b][i]) lower |= 1;
       }

            
      if (gameoverFlag == true)
      {  
        elapsedTime = millis() - startTime;

        // Display random pattern for pre-defined period before blanking display
        if (elapsedTime < 2000)
        {            
          bi_maxTransferSingle(RED, 1, i,  random(255));
          bi_maxTransferSingle(RED, 2, i,  random(255));
          
          bi_maxTransferSingle(GREEN, 1, i, random(255));
          bi_maxTransferSingle(GREEN, 2, i, random(255));
      
          cnt = cnt + 1;
          
          if (cnt > 80)
          {
            TriggerSound();
            TriggerSound();
            cnt = 0;
          }
        }   
        else
        {
          bi_maxTransferSingle(RED, 1, i, 0x00);  // clear
          bi_maxTransferSingle(RED, 2, i, 0x00);  // clear

          bi_maxTransferSingle(GREEN, 1, i, 0x00);  // clear
          bi_maxTransferSingle(GREEN, 2, i, 0x00);  // clear        
        }  
      
      }
      else
      {
        if (selectColor == RED)
        {
          bi_maxTransferSingle(GREEN, 1, i, lower);
          bi_maxTransferSingle(GREEN, 2, i, upper);
        }
        else
        {
          bi_maxTransferSingle(RED, 1, i, lower);
          bi_maxTransferSingle(RED, 2, i, upper);  
        }        
      }      
    } 
    
    
    
    if (gameoverFlag == false)
    {  
      // For pile - to display orange    
      for(i=0;i<8;i++)
      {      
         byte upper = 0;
         int b;
         for(b = 0;b<8;b++)
         {
           upper <<= 1;
           if (tmppileUpper[b][i]) upper |= 1;
         }
       
       
         byte lower = 0;
         for(b = 0;b<8;b++)
         {
           lower <<= 1;
           if (tmppileLower[b][i]) lower |= 1;
         }

      
        // To alternate color of new block between RED and GREEN
        if (selectColor == RED)
        {
          bi_maxTransferSingle(RED, 1, i, lower);
          bi_maxTransferSingle(RED, 2, i, upper);
        }
        else
        {
          bi_maxTransferSingle(GREEN, 1, i, lower);
          bi_maxTransferSingle(GREEN, 2, i, upper);    
        }  

      }         
    }    
}



//**********************************************************************************************************************************************************  
// Change Max72xx brightness
void setBrightness()
{      
    bi_maxTransferAll(0x0A, SetbrightnessValue);      //Set Brightness
    bi_maxTransferAll(0x00, 0x00);  //No-op commands
}



//**********************************************************************************************************************************************************  
// Clear Display
void clearDisplay(uint8_t whichColor) //whichColor = 1 for RED, 2 for GREEN
{     
    for (int y=0; y<8; y++) {
      bi_maxTransferSingle(whichColor, 1, y, 0); //Turn all Off  //For X1 LED matrix Game
      bi_maxTransferSingle(whichColor, 2, y, 0); //Turn all Off  //For X1 LED matrix Game
    }
}



//**********************************************************************************************************************************************************  
/**
 * Transfers data to a MAX7219/MAX7221 register of a particular Bi-color LED Matrix module.
 *
 * @param whichMax The Max72xx to load data and value into
 * @param address The register to load data into
 * @param value Value to store in the register
 */
//**********************************************************************************************************************************************************  
void bi_maxTransferAll(uint8_t address, uint8_t value) {
  stopISR();
  digitalWrite(SPI_CS, LOW); 

    for ( int c=1; c<= bi_maxInUse*2;c++) {
        SPI.transfer(address);  // specify register
        SPI.transfer(value);  // put data
    }

  digitalWrite(SPI_CS, HIGH); 
  startISR();
}


 
//**********************************************************************************************************************************************************  
void bi_maxTransferOne(uint8_t whichMax, uint8_t address, uint8_t value) {

  byte noop_reg = 0x00;    //max7219 No op register
  byte noop_value = 0x00;  //value

  stopISR();
  digitalWrite(SPI_CS, LOW); 

  for (int i=bi_maxInUse; i>0; i--)   // Loop through our number of Bi-color LED Matrices 
  {
    if (i==whichMax)
    {
      SPI.transfer(address);  // Send the register address
      SPI.transfer(value);    // Send the value

      SPI.transfer(address);  // Send the register address
      SPI.transfer(value);    // Send the value

    }
    else
    {
      SPI.transfer(noop_reg);    // Send the register address
      SPI.transfer(noop_value);  // Send the value

      SPI.transfer(noop_reg);    // Send the register address
      SPI.transfer(noop_value);  // Send the value
    }
  }

  digitalWrite(SPI_CS, HIGH);
  startISR();
}



//**********************************************************************************************************************************************************  
void bi_maxTransferSingle(uint8_t whichColor, uint8_t whichMax, uint8_t address, uint8_t value) {  //whichColor = 1 for RED, 2 for GREEN

  byte noop_reg = 0x00;    //max7219 No op register
  byte noop_value = 0x00;  //value

  stopISR();
  digitalWrite(SPI_CS, LOW); 


if (whichColor==GREEN)
{
  for (int i=bi_maxInUse; i>0; i--)   // Loop through our number of Bi-color LED Matrices 
  {
    if (i==whichMax)
    {
      SPI.transfer(address+1);   // Send the register address
      SPI.transfer(value);       // Send the value

      SPI.transfer(noop_reg);    // Send the register address
      SPI.transfer(noop_value);  // Send the value

    }
    else
    {
      SPI.transfer(noop_reg);    // Send the register address
      SPI.transfer(noop_value);  // Send the value

      SPI.transfer(noop_reg);    // Send the register address
      SPI.transfer(noop_value);  // Send the value
    }
  }
}
else
{
  for (int i=bi_maxInUse; i>0; i--)   // Loop through our number of Bi-color LED Matrices 
  {
    if (i==whichMax)
    {
      SPI.transfer(noop_reg);    // Send the register address
      SPI.transfer(noop_value);  // Send the value

      SPI.transfer(address+1);   // Send the register address
      SPI.transfer(value);       // Send the value

    }
    else
    {
      SPI.transfer(noop_reg);    // Send the register address
      SPI.transfer(noop_value);  // Send the value

      SPI.transfer(noop_reg);    // Send the register address
      SPI.transfer(noop_value);  // Send the value
    }
  }
}


  digitalWrite(SPI_CS, HIGH);
  startISR();
}



//**********************************************************************************************************************************************************  
void bi_maxShutdown(uint8_t cmd) 
{
  byte noop_reg = 0x00;      //max7219_reg_no_op
  byte shutdown_reg = 0x0c;  //max7219_reg_shutdown
  byte col = 0x01;  //shutdown false
  byte col2 = 0x00;  //shutdown true


  if (cmd == offREDoffGREEN)
  {    
    stopISR();
    digitalWrite(SPI_CS, LOW);

    for (int c =1; c<= bi_maxInUse; c++)
    {
      SPI.transfer(shutdown_reg);  // Send the register address
      SPI.transfer(col2);          // Send the value

      SPI.transfer(shutdown_reg);  // Send the register address
      SPI.transfer(col2);          // Send the value
    }

    digitalWrite(SPI_CS, HIGH);    
    startISR();
  }
  else if (cmd == offREDonGREEN)
  {
    stopISR();
    digitalWrite(SPI_CS, LOW);

    for (int c =1; c<= bi_maxInUse; c++) 
    {
      SPI.transfer(shutdown_reg);  // Send the register address
      SPI.transfer(col);           // Send the value

      SPI.transfer(shutdown_reg);  // Send the register address
      SPI.transfer(col2);          // Send the value
    }

    digitalWrite(SPI_CS, HIGH);
    startISR();
  }
  else if (cmd == onREDoffGREEN)
  {
    stopISR();
    digitalWrite(SPI_CS, LOW);

    for (int c =1; c<= bi_maxInUse; c++) 
    {      
      SPI.transfer(shutdown_reg);  // Send the register address
      SPI.transfer(col2);          // Send the value

      SPI.transfer(shutdown_reg);  // Send the register address
      SPI.transfer(col);           // Send the value  
    }

    digitalWrite(SPI_CS, HIGH);
    startISR();
  }



  //No ops register to shift out instructions   
  stopISR();
  digitalWrite(SPI_CS, LOW);

  for (int c =1; c<= bi_maxInUse; c++) 
  {      
    SPI.transfer(noop_reg);  // Send the register address
    SPI.transfer(0x00);      // Send the value

    SPI.transfer(noop_reg);  // Send the register address
    SPI.transfer(0x00);      // Send the value
  }

  digitalWrite(SPI_CS, HIGH);
  startISR();
}



//**********************************************************************************************************************************************************
void altShutDown()    //alternate shutdown of MAX7219 chips for RED and GREEN LEDs 
{
  if (colorMode == '3')    //Scrolling in ORANGE
  {
    if(maxInShutdown==RED){
      bi_maxShutdown(onREDoffGREEN);
      maxInShutdown=GREEN;
    } 
    else 
    { 
      bi_maxShutdown(offREDonGREEN);
      maxInShutdown=RED;
    }
  }
  else if (colorMode == '2')   //Scrolling in GREEN
  {
    bi_maxShutdown(offREDonGREEN);
    maxInShutdown=RED;
  }
  else if (colorMode == '1')   //Scrolling in RED
  {
    bi_maxShutdown(onREDoffGREEN);
    maxInShutdown=GREEN;
  }
  else if (colorMode == '4')  //Blank Display
  {
    bi_maxShutdown(offREDoffGREEN);       
    maxInShutdown=GREEN;
  }
} 



//**********************************************************************************************************************************************************
/////////////////////////////ISR Timer Functions using Timer2///////////////////////////
ISR(TIMER2_COMPA_vect) {  //This ISR toggles shutdown between the 2MAX7221's

  if (colorMode == '3')    // ORANGE
  {
    if(maxInShutdown==RED){
      bi_maxShutdown(onREDoffGREEN);
      maxInShutdown=GREEN;
    } 
    else 
    { 
      bi_maxShutdown(offREDonGREEN);
      maxInShutdown=RED;
    }
  }
  else if (colorMode == '2')   // GREEN
  {
    bi_maxShutdown(offREDonGREEN);
    maxInShutdown=RED;
  }
  else if (colorMode == '1')   // RED
  {
    bi_maxShutdown(onREDoffGREEN);
    maxInShutdown=GREEN;
  }
  else if (colorMode == '4')  //Blank Display
  {
    bi_maxShutdown(offREDoffGREEN);       
    maxInShutdown=GREEN;
  }
} 



//**********************************************************************************************************************************************************
void setISRtimer() // setup ISR timer controling toggleing
{ 
  TCCR2A = 0x02;                        // WGM22=0 + WGM21=1 + WGM20=0 = Mode2 (CTC)
  TCCR2B = 0x05;                // CS22=1 + CS21=0 + CS20=1 = /128 prescaler (125kHz)
  TCNT2 = 0;                            // clear counter
  OCR2A = ISR_FREQ;                     // set TOP (divisor) - see #define
}
     
 
     
//**********************************************************************************************************************************************************
void startISR()    // Starts the ISR
{
  TCNT2 = 0;                            // clear counter (needed here also)
  TIMSK2|=(1<<OCIE2A);                  // set interrupts=enabled (calls ISR(TIMER2_COMPA_vect)
}

  
     
//**********************************************************************************************************************************************************
void stopISR()    // Stops the ISR
{
  TIMSK2&=~(1<<OCIE2A);                  // disable interrupts
}



//**********************************************************************************************************************************************************
void TriggerSound()
{
  // Set up a counter to pull from melody[] and beats[]
  for (int i=0; i<MAX_COUNT; i++) {
    tone_ = melody[i];
    beat = beats[i];

    duration = beat * tempo; // Set up timing

    playTone();
    delayMicroseconds(pause);
  }
}



//**********************************************************************************************************************************************************
// Pulse the speaker to play a tone for a particular duration
void playTone() 
{
  long elapsed_time = 0;
  if (tone_ > 0) { // if this isn't a Rest beat, while the tone has
    //  played less long than 'duration', pulse speaker HIGH and LOW
    while (elapsed_time < duration) {

      digitalWrite(speakerOut,HIGH);
      delayMicroseconds(tone_ / 2);

      // DOWN
      digitalWrite(speakerOut, LOW);
      delayMicroseconds(tone_ / 2);

      // Keep track of how long we pulsed
      elapsed_time += (tone_);
    }
  }
  else { // Rest beat; loop times delay
    for (int j = 0; j < rest_count; j++) { // See NOTE on rest_count
      delayMicroseconds(duration);  
    }                                
  }                                
}
