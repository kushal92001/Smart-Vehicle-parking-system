#include<LiquidCrystal.h>

#define ECHO_1_PIN 3
#define TRIG_1_PIN 4
#define ECHO_2_PIN 6
#define TRIG_2_PIN 7
#define ECHO_3_PIN 9
#define TRIG_3_PIN 8
#define ECHO_4_PIN 11
#define TRIG_4_PIN 12

#define LCD_RS_PIN A5
#define LCD_E_PIN A4
#define LCD_D4_PIN 5
#define LCD_D5_PIN 10
#define LCD_D6_PIN A3
#define LCD_D7_PIN A2

#define BUTTON_PIN 2
#define INF 9999
//lcd
LiquidCrystal lcd(LCD_RS_PIN,LCD_E_PIN,LCD_D4_PIN,
                  LCD_D5_PIN,LCD_D6_PIN,LCD_D7_PIN);

//Ultrasonic sensor
unsigned long lastTimeUltrasonicTrigger = millis();
unsigned long ultrasonicTriggerDelay = 60;
double USdistance[4]={0.0,0.0,0.0,0.0};
int parkingSlots[4] = {1, 1, 1, 1};
String parkingSize[4] = {"Small","Mid","Small","Mid"};
byte US_TRIG_Array[4]={TRIG_1_PIN,TRIG_2_PIN,TRIG_3_PIN,TRIG_4_PIN};
byte US_ECHO_Array[4]={ECHO_1_PIN,ECHO_2_PIN,ECHO_3_PIN,ECHO_4_PIN};

unsigned long pulseInTimeBegin[4];
unsigned long pulseInTimeEnd[4];
volatile bool newDistanceAvailable[4] = {false,false,false,false};

double measureDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  return pulseIn(echoPin, HIGH) / 58.0; // Convert the time to distance (cm)
}

void triggerUltrasonicSensor()
{
  for(int i=0;i<4;i++)
  {
    digitalWrite(US_TRIG_Array[i], LOW);
    delayMicroseconds(2);
    digitalWrite(US_TRIG_Array[i], HIGH);
    delayMicroseconds(10);
    digitalWrite(US_TRIG_Array[i], LOW);
  }
}

double getUltrasonicDistance(int i)
{ 
  	double durationMicros = pulseInTimeEnd[i]-pulseInTimeBegin[i];
    USdistance[i]=durationMicros/58.0;
    return USdistance[i];
}

void updateParkingStatus() {
  for (int slot = 0; slot < 4; slot++) {
    if (USdistance[slot] < 200){
      parkingSlots[slot] = 0; // Mark the slot as occupied
    } else {
      parkingSlots[slot] = 1; // Mark the slot as empty
    }
  }
}

//push button
unsigned long lastTimeButtonChanged = millis();
unsigned long buttonDebounceDelay = 50;
byte buttonState;

//parking slot
int entranceSlot=0;
const int numSlots=5;

int graph[numSlots][numSlots];
bool visited[numSlots];
int distance[numSlots];

void initializeGraph()
{
  for(int i=0;i<numSlots;i++)
    for(int j=0;j<numSlots;j++)
    	if(i==j)
    		graph[i][j]=INF;
  graph[0][1]=graph[1][0]=3;
  graph[0][2]=graph[2][0]=INF;
  graph[0][3]=graph[3][0]=3;
  graph[0][4]=graph[4][0]=INF;
  graph[1][2]=graph[2][1]=1;
  graph[1][3]=graph[3][1]=INF;
  graph[1][4]=graph[4][1]=INF;
  graph[2][3]=graph[3][2]=INF;
  graph[2][4]=graph[4][2]=INF;
  graph[3][4]=graph[4][3]=1;
}

void dijkstra(int start){
  //int distance[numSlots];
  
  bool visited[numSlots]={false,false,false,false,false};
  int pred[numSlots];
  int mindistance=INF,nextnode,i,j;
  for(i=0;i<numSlots;i++)
  {
    distance[i]=graph[start][i];
    pred[i]=start;
    visited[i]=false;
  }
  distance[start]=0;
  visited[start]=true;
  int count=1;
  while(count<numSlots-1){
    mindistance=INF;
    for(i=0;i<numSlots;i++){
    	if(distance[i]<mindistance && visited[i]==false)
    	{
      	  mindistance=distance[i];
          nextnode=i;
    	}
    }
    visited[nextnode]=true;
    
    for(i=0;i<numSlots;i++){
      if(visited[i]==false){
         if(mindistance + graph[nextnode][i] < distance[i])
         {
        	distance[i]=mindistance+graph[nextnode][i];
        	pred[i]=nextnode;
         }
      }
    }
    count++;
  }
  for(i=0;i<numSlots;i++)
  {
    if(i!=start)
    {
      //Serial.print(i);
      Serial.println(distance[i]);
    }
  }
}
void findSlot(String carType)
{
  int mindist=INF,slotpos=-1;
  for(int i=0;i<numSlots;i++)
  {
    if(i!=entranceSlot)
    {
      if(distance[i]<mindist){
        if(parkingSlots[i-1]==1){//free
          if((carType.equals("midsize") && parkingSize[i-1].equals("Mid"))
             ||(carType.equals("compact")&& parkingSize[i-1].equals("Small"))){//length
        		mindist=distance[i];
        		slotpos=i;
             }
          }
        }
      }
   }
   if(slotpos!=-1){
       parkingSlots[slotpos-1]=0;//slot reserved
       lcd.setCursor(0,0);
       lcd.print("Parking slot:");
       lcd.print(slotpos);
       lcd.print("  ");
       lcd.setCursor(0,1);
       lcd.print("Distance:");
       lcd.print(mindist);
       lcd.print("m    ");
   }
   else
   {
     lcd.setCursor(0,0);
     lcd.print("Parking slot: NA");
     lcd.setCursor(0,1);
     lcd.print("Distance: NA    ");
   }            
}
void setup()
{
  Serial.begin(9600);
  lcd.begin(16,2);
  for(int i=0;i<4;i++)
  {
    pinMode(US_ECHO_Array[i], INPUT);
    pinMode(US_TRIG_Array[i], OUTPUT);
  }
  
  pinMode(BUTTON_PIN, INPUT);
  buttonState = digitalRead(BUTTON_PIN);
  
  lcd.print("Initializing...");
  delay(1000);
  lcd.clear();
  initializeGraph();
}

void loop()
{
  unsigned long timeNow = millis();
  if(timeNow - lastTimeUltrasonicTrigger > ultrasonicTriggerDelay)
  {
    lastTimeUltrasonicTrigger+=ultrasonicTriggerDelay;
    for (int slot = 0; slot < 4; slot++) {
      double distance = measureDistance(US_TRIG_Array[slot], US_ECHO_Array[slot]);
      USdistance[slot]=distance;
      //Serial.println(slot);
      //Serial.println(distance);
    }
    updateParkingStatus();
    /*Serial.print(parkingSlots[0]);
    Serial.print(parkingSlots[1]);
    Serial.print(parkingSlots[2]);
    Serial.print(parkingSlots[3]);*/
  }
  
  if(timeNow - lastTimeButtonChanged > buttonDebounceDelay){
    byte newButtonState = digitalRead(BUTTON_PIN);
    if(newButtonState !=buttonState){
      lastTimeButtonChanged = timeNow;
      buttonState = newButtonState;
      if(buttonState == LOW){
        Serial.println("Find slot");
        dijkstra(entranceSlot);
        //Serial.println(nearestEmptySlot+1);
        //parkingSlots[nearestEmptySlot]=0;
        Serial.println("Enter your car type as compact or midsize");
        while(Serial.available() == 0){}
        String carType=Serial.readString();
        Serial.println(carType);
        findSlot(carType);
      }
    }
  }
}
