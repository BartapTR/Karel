#include <iostream>
#include <vector>
#include <array>
#include <cmath>
#include <string>
#include <cctype>
#include <algorithm>

using std::vector;
using std::array;
using std::cout;
using std::string;
using std::endl;

// A position always has exactly three coordinates. std::array stores its
// elements directly (unlike vector, it cannot grow or shrink), so it suits
// fixed-size data such as one XYZ position.
using Position = array<float, 3>;
using strVec = vector<string>;
strVec functions;
strVec functionen;

// getClosestBallInfo returns one compact snapshot with this fixed layout:
// [distance, delta X, delta Y, delta Z, index in balls].
// Keeping related values together means callers can find the nearest ball
// once and reuse all of its data instead of rescanning the vector per value.
using ClosestBallInfo = array<float, 5>;

// Named offsets make the array's layout readable at each use site and avoid
// scattering unexplained numeric subscripts such as closestBall[3].
constexpr size_t DISTANCE_INDEX = 0;
constexpr size_t DELTA_X_INDEX = 1;
constexpr size_t DELTA_Y_INDEX = 2;
constexpr size_t DELTA_Z_INDEX = 3;
constexpr size_t BALL_INDEX = 4;

/*Use both these two index groups for conditionals, but the top only for getting info
constexpr size_t Movement_Count_INDEX = 0;
constexpr size_t Direction_INDEX = 1;
constexpr size_t Speed_INDEX = 2;
*/

string lastCommand;

Position position = {0, 0, 0};
vector<Position> balls = {};
//the shape of area is a sphere

//After that you can add numbers and command codes
//Maybe gravity, time and saving
//Turn it into an actual game?

const float radius = 30.0;

int xydirection = 0;
int xzdirection = 0;
int normalMovement = 1.0;
const double PI = 3.1416;

float RMS(vector<float> numbas){
    float rms;
    for (float i:numbas){
        rms += i*i;
    }
    rms = std::sqrt(rms);
    return rms;
}

int powar(int numba, int powa){
    int out = 1;
    if (powa == 0){
        return out;
    }
    while(powa >1){
        if (powa%2 == 0){
            numba *= numba;
            powa /= 2;
        }else if (powa >1){
            powa-=1;
            out *= numba;
        }
    }
    out *= numba;
    return out;
}

int toBinary(string inphut){
    int numbir = 0;
    for(int i=0;i<inphut.length();i++){
        if (inphut[i]=='0'){
            continue;    
        }
        numbir += (inphut[i]-'0')*powar(2, inphut.length()-i-1);
        //cout << inphut[i] <<"  "<< i << " " << numbir << endl;
    }
    return numbir;
}

void placeBall(){
    // Both position and each entry in balls use the same three-coordinate
    // type, so this copies the current XYZ position into the dynamic vector.
    balls.push_back(position);
}

ClosestBallInfo getClosestBallInfo();

float getBall(){
    // Store the returned array locally: all values below refer to the same
    // closest-ball search, and the vector is not searched again for its index.
    const ClosestBallInfo closestBall = getClosestBallInfo();
    const float distance = closestBall[DISTANCE_INDEX];
    cout << "Distance to ball: " << distance << endl;
    // A negative distance is the "no ball" sentinel. Check it before using
    // the returned index, because an empty vector has no valid element to erase.
    if (distance >= 0 && distance <= 5){
        cout << " Got Ball"<<endl;
        // The lookup stores the vector index as a float to fit the all-float
        // result array; convert it back before using it to index the vector.
        const size_t ballIndex = static_cast<size_t>(closestBall[BALL_INDEX]);
        balls[ballIndex] = balls.back();
        balls.pop_back();
    }
    return distance;
}

float autoGetBall(){
    const ClosestBallInfo closestBall = getClosestBallInfo();
    // The lookup returns a negative distance when balls is empty. Returning
    // here prevents using missing offsets or dividing by an invalid distance.
    if (closestBall[DISTANCE_INDEX] < 0){
        cout << "No balls available to collect." << endl;
        return -1;
    }

    string line;
    cout << "How many times to move to get to ball: ";
    std::getline(std::cin,line);
    const int steps = stoi(line);
    if (steps <= 0){
        cout << "Step count must be > 0" << endl;
        return -1;
    }
    
    const float deltaY = closestBall[DELTA_Y_INDEX];
    const float deltaX = closestBall[DELTA_X_INDEX];
    const float deltaZ = closestBall[DELTA_Z_INDEX];
    const vector<float> vec = {deltaX,deltaY,deltaZ};
    const float hypotenuse =  RMS(vec);
    // If all offsets are zero, the player is already at the target. Avoid
    // dividing by this zero length while calculating the turn angle.
    if (hypotenuse == 0){
        cout << "Already at the closest ball." << endl;
        return 0;
    }
    
    const float requiredSpeed = hypotenuse/steps;
    int requiredTurn;
    
    cout << endl << "Dy, Dx, Dz " << deltaY << ", " << deltaX << ", " << deltaZ;
    
    if (deltaY<0){//
        int degree = std::asin(-deltaY/hypotenuse)*180/PI;
        cout << endl <<"degree: "<<degree<<"  ";
        if (deltaX<0){//-dx -dy
            int targetDirection = 180+degree;
            requiredTurn = targetDirection-xydirection;
        }else{//dx -dy
            int targetDirection = 350-degree;
            requiredTurn = targetDirection-xydirection;
        }
    }else{//positive player is below
        int degree = std::asin(deltaY/hypotenuse)*180/PI;
        if (deltaX<0){//-dx dy
            int targetDirection = 180-std::asin(deltaY/hypotenuse)*180/PI;
            requiredTurn = targetDirection-xydirection;
        }else{//dx dy
            requiredTurn = degree-xydirection;
        }
    }   
    if (hypotenuse !=-1){
        cout << endl << "To get to the closest ball, set speed to " << requiredSpeed << " and turn " << requiredTurn<<" 'it's "<<requiredTurn/19<<endl;
    }
    return requiredTurn;
}

ClosestBallInfo getClosestBallInfo(){
    // A distance of -1 signals that no ball was found. The other sentinel
    // values keep the whole result initialized until a candidate is selected.
    ClosestBallInfo closestBall = {-1, -1, -1, -1, -1};
    // INFINITY ensures the first real distance is smaller, including when
    // there is exactly one ball or its distance is zero.
    float shortestDistance = INFINITY;

    // Calculate distance and offsets together for each ball. When a nearer
    // ball is found, save all its values in one result; this is one O(n) scan.
    for (size_t ballIndex = 0; ballIndex < balls.size(); ++ballIndex){
        const float deltaX = balls[ballIndex][0] - position[0];
        const float deltaY = balls[ballIndex][1] - position[1];
        const float deltaZ = balls[ballIndex][2] - position[2];
        const vector<float> veco = {deltaX,deltaY,deltaZ};
        const float distance = RMS(veco);
        //sqrt(deltaX*deltaX + deltaY*deltaY + deltaZ*deltaZ);

        if (distance < shortestDistance){
            shortestDistance = distance;
            // Aggregate initialization fills the result in the documented
            // order above, with an explicit conversion for the vector index.
            closestBall = {
                distance,
                deltaX,
                deltaY,
                deltaZ,
                static_cast<float>(ballIndex)
            };
        }
    }

    // Returning std::array by value gives the caller an independent,
    // fixed-size copy of the result (small arrays like this are inexpensive).
    return closestBall;
}
void move(){
    float radi = -xydirection*PI/180;
    float gadi = -xzdirection*PI/180;
    const vector<float> vecc = {1,std::tan(radi),std::tan(gadi)};
    float xM= normalMovement/RMS(vecc);
    float yM=xM*std::tan(radi);
    float zM=xM*std::tan(gadi);
    float t1 = position[0]+round(100.0*xM)/100.0;
    float t2 = position[1]+round(100.0*yM)/100.0;
    float t3 = position[2]+round(100.0*zM)/100.0;
    if (std::abs(t1)>= radius){
        t1 = radius*t1/std::abs(t1)-0.01f;
        cout <<endl<< "Caution: Hit Wall X"<<endl<<endl;
    }
    if (std::abs(t2)>=radius){
        t2 = radius*t2/std::abs(t2)-0.01f;
        cout <<endl<<"Caution: Hit Wall Y"<<endl<<endl;
    }
    if (std::abs(t3)>=radius){
        t3 = radius*t3/std::abs(t3)-0.01f;
        cout <<endl<<"Caution: Hit Wall Z"<<endl<<endl;
        
    }
    position[0] = t1;
    position[1] = t2;
    position[2] = t3;
}

/*Bak adam bize diyecek ki 009A donmek istiyorum
Biz bunu 0x19+0 xz yonunde 0,   9x19+10 181 xy yonunde donmek
istiyourm diye anliyacagiz*/

int giveNona(string binput){
    //binput here is B9 or JJ type 2 digits
    char rep[9]= {'A','B','C','D','E','F','G','H','I'};
    int nums[9] = {10 ,11 ,12 ,13 ,14 ,15 ,16 ,17 ,18};
    int convertedDegrees = 0;

    //Actual number for the digit, *19^placement from the left
    if (std::isdigit(binput[0])){
        convertedDegrees += (binput[0]-'0')*19;
    }if(std::isdigit(binput[1])){
        convertedDegrees += (binput[1]-'0');
    }
    //Letter as the digit, find and use
    for (int i=0;i<9;i++){
        if (binput[0] == rep[i]){
            convertedDegrees += nums[i]*19;
        }
        if (binput[1] == rep[i]){
            convertedDegrees += nums[i];
        }
    }
    return convertedDegrees;
}

void turn(string degres){
    int xzdegre = 0;
    int xydegre = 0;
    
    std::transform(degres.begin(),degres.end(),degres.begin(),[](unsigned char c){return std::toupper(c);});
    if (degres.length()==4){
        string xyx = degres.substr(0, 2);
        xzdegre = giveNona(xyx);
        string xzx = degres.substr(2, 2);
        xydegre = giveNona(xzx);
    }
    xydirection += xydegre;
    xydirection %= 360;
    
    xzdirection +=xzdegre;
    xzdirection %= 360;
}

void setSpeed(string newSpeed){
    int num = normalMovement;
    num = toBinary(newSpeed);
    normalMovement = num;
    cout << normalMovement << endl;
}

float getDistance();

void giveInfo(){
    cout << endl << "You are at: "<<position[0]<<", "<<position[1]<<", "<<position[2]<< endl<< "xydirection: "<< xydirection<< " Degrees"<< endl<< "xzdirection: "<< xzdirection<< " Degrees"<< endl << "Speed: " << normalMovement<< endl << "Distance from origin: " << 0.01*round(100*getDistance());
    // This display only needs the distance, so it reads that field from the
    // same result type used by getBall and autoGetBall.
    cout << endl << "Distance to closest Ball: " << getClosestBallInfo()[DISTANCE_INDEX]<<endl<<"Number of balls: "<<balls.size()<<endl<< endl;
}
string checkCommand(string cmd);
float getDistance(){
    float n1 = position[0];
    float n2 = position[1];
    float n3 = position[2];
    const vector<float> vecco = {n1,n2,n3};
    return RMS(vecco);
}

/*string conditionalCheck(string conditionStatement){
    //The conditional statement is a 4 digit input, first 2 digits are the index of the value to check, the next 2 digits are the value to check for
    //The index is based on the order of the values in the giveInfo function
    //The value to check for is a 4 digit binary input
    int index = toBinary(conditionStatement.substr(0,2));
    int valueToCheckFor = toBinary(conditionStatement.substr(2,2));
    int valueToCheck;
    if (index == Movement_Count_INDEX){
        valueToCheck = balls.size();
    }else if(index == Direction_INDEX){
        valueToCheck = xydirection;
    }else if(index == Speed_INDEX){
        valueToCheck = normalMovement;
    }
    if (valueToCheck == valueToCheckFor){
        return "T";
    }else{
        return "F";
    }

}
*/


bool checkCond (string cond){
	//const int Indexes[] = {normalMovement, xydirection, getDistance(), balls.size(), getClosestBallInfo()[DISTANCE_INDEX], position[0], position[1], position[2]};
	const int Indexes[8] = {0,1,2,3,4,5,6,7};
	const char Comparisons[4] = {'<','>','=','!'};

	const int Index = Indexes[toBinary(cond.substr(0, 3))];
	const char comparison = Comparisons[toBinary(cond.substr(3, 3))];
	const int value = toBinary(cond.substr(6, 3));

    cout << "Index: " << cond.substr(0, 3) << " Comparison: " << toBinary(cond.substr(0, 3)) << " Value: " << Indexes[toBinary(cond.substr(0, 3))] << endl;
    cout << "Index: " << cond.substr(3, 3) << " Comparison: " << toBinary(cond.substr(3, 3)) << " Value: " << Comparisons[toBinary(cond.substr(3, 3))] << endl;
    cout << "Index: " << cond.substr(6, 3) << " Comparison && Value is: " << toBinary(cond.substr(6, 3)) << endl;

    cout <<  Index << "  "<< comparison << "  "<< value << endl;

	if (comparison == '<'){
		if (Index < value){
            return true;
        }else{
	        return false;
        }
    }
	if (comparison == '>'){
		if (Index > value){
            return true;
        }else{
	        return false;
        }
    }
	if (comparison == '='){
		if (Index == value){
            return true;
        }else{
	        return false;
        }
    }
	if (comparison == '!'){
		if (Index != value){
            return true;
        }else{
	        return false;
        }
    }
}

string CondEval (string statement){
	bool IsCond = true;
	string nonCondStatement;
	string condition = "";
	string newCond = "";
	vector<bool> isTrue;
	int isNested = 0;
	vector<string> operations;

    //000 001 010 100 100 011 110 000
    // 0   >   2   OR  4   !   6  move
    // 1   2   3   4   5   6   7   8

    cout << "Statement: " << statement << endl;
	for (int i = 0; i < statement.length()/3; i++){
		string Cmd = statement.substr(3*i, 3);
        if (Cmd == "111" && IsCond == false){
			isNested++;
		}
		if (Cmd == "con"){
            isNested--;//make it an int
            if (isNested == 0) {
                nonCondStatement += CondEval(newCond);
                newCond = "";
            }
		}
		if (IsCond && isNested == 0){             //if((i+1)%4==1)//Index	if((i+1)%4==2)//comparison op	if((i+1)%4==3)//Value to compare against
			if((i+1)%4==0){      //Comparison operator between conditions
				if(Cmd != "100" && Cmd != "101"){//not OR or AND
                    if(Cmd != "111") {//fixes a problem where the nested conditional is right after the condition of the parent conditional
                        nonCondStatement += Cmd;
                    }else{
                        isNested++;
                    }
					isTrue.push_back(checkCond(condition));
					condition = "";
                    IsCond=false;//end of conditional statement
                }else{//OR or AND
                    isTrue.push_back(checkCond(condition));
					condition = "";
                    //end of one condition
	                operations.push_back(Cmd);
                }
            }else{//Index, comparison operator, value to compare against
				condition +=Cmd;
            }
		}else if(isNested != 0){
			if(Cmd != "111") newCond += Cmd;
        }else{
           if(Cmd != "con") nonCondStatement += Cmd;
        }
    }

    cout << "isTrue: ";
    for (bool b : isTrue){
        cout << b << " ";
    }
    cout << endl << "operations: ";
    for (string s : operations){
        cout << s << " ";
    }
    vector<bool> newIsTrue = isTrue;
	if (operations.size()>0){           //–Repeatedly Check/change cont if isTrue[i] is operations[i] with isTrue[i+1] 
		vector<string> newOperations = operations;
		while (newIsTrue.size() != 1){
            cout << "iteration" << endl;
			bool second= newIsTrue[1];
			string oper = newOperations[0];
            newIsTrue[1] = newIsTrue.back();
            newIsTrue.pop_back();
            newOperations[0] = newOperations.back();
            newOperations.pop_back();
            cout << newIsTrue[0] << "  " << oper << "  " << second << endl;
			if (oper=="100"){
				if(newIsTrue[0]==1 || second==1){
					newIsTrue[0] = true;	
                }else{
					newIsTrue[0] = false;		
                }	
            }if (oper=="101"){
				if(newIsTrue[0]==1 && second==1){
					newIsTrue[0] = true;
				}else{
                    cout << "false";
					newIsTrue[0] = false;
				}
			}
            cout << endl << "1st item in newIsTrue: " << newIsTrue[0] << endl;
        }
        cout << "Final isTrue size and then content: " << newIsTrue.size() << endl;
        for (bool b : newIsTrue){
            cout << b << " ";
        }
        if (newIsTrue[0] == 0){
            nonCondStatement = "";
        }
        cout << endl << "nonCondStatement: " << nonCondStatement << endl;
        return nonCondStatement;
	}else{
        if(isTrue[0] == 0){
            nonCondStatement = "";
        }
        cout << endl << "nonCondStatement: " << nonCondStatement << endl;
        return nonCondStatement;
    }
}

string nextCommand(string gonogo, bool isConditionalen){
    string input;
    if(gonogo.length()<2){
        if(gonogo != "1") cout << "Enter next command: ";
        std::getline(std::cin, input);
    }else{
        input = gonogo;   
    }

    cout << "Input: " << input << endl;

    bool end = 0;
    int spec = 0;

    /*
    So since functions have been added, the next thing is to add conditions and conditionals
    the part about getting things between 2 commands is the same, but conditionals will be a bit different, 
    The reason for that is the program currently doesn't have variables or anything like that
    Also it's somewhat unrelated to use/add conditionals; however, I think I found the reason: it is to check for position, speed, direction, or
    distance from origin. So the conditionals will be based on those things, and the commands will be based on those things as well. 
    What I am thinking is that we use indexes for the conditionals. Will the user be able to create new/their own conditionals?
    They should be, eventually, but for now, the conditionals will be based on the current things that can be checked.
    As variables can be used to create better loops, right now you can set speed to 1 and move until position is exactly 10 units away from the origin, as a way to
    crete a loop(you also need to turn to the right direction, but that is not important). 

    We can just use existing infastructure(4 digit input) 1. index, 2. true or false, 3. how many commands to execute(this might push towards functions instead of specOps)
    If we went that route, the index becomes harder or not possible: con then what?
    Do we have to mix them?
    Ok so this is the way to do it: con(0111)000putcon
    disecting gives:
    con--initializes both the end and the specCom
    (0 False
    111 8th index
    000 move if false
    put put if false
    con--closes cond and specCom
    */
    string specComs;
    string funcSpec;
    string funcs;

    int nests = 0;

    bool isConditional = false;
    //To make sure the conditional works with functions as well

    //fun 111 (010 110 111) 100 (011 000 101) 001(004e) 000 000 000 fun
    //fun con  speed>= 7/n  and   direct==90  turnxy90  mov mov mov fun

    //So construct a function exactly the same, but when it is called, a conditional c++ function is called and the conditional is checked,
    //if true, the actual command part is resent here 

    for (size_t ii=0; ii<input.length()/3;ii++){
        string commaned;
        if ((spec == 0 && end == 0) || end == 1){
            commaned = input.substr(ii*3, 3);
        }else if(spec == 1 || spec == 11 || spec == 21){//ignore the first char
            specComs += input.substr(ii*3+1, 2);
            spec += 1;
        }else if(spec == 2 || spec == 12 || spec == 22){//ignore the last char
            specComs += input.substr(ii*3, 2);
            if(end == 0){
                if (spec == 2){//execute turn command
                    turn(specComs);
                }else if(spec == 12){//execute speed command
                    setSpeed(specComs);
                }
            }
            //reset specComs and spec
            specComs = "";
            spec = 0;
        }
        if (spec == 0){
            if (commaned == "001"){
                spec = 1;
            }else if(commaned == "010"){
                spec = 11;
            }
        }
        //This creates functions with commands in between "fun" commands
        //The user inputs another vector with names of functions, uses those names to call the functions
        if (end == 1){//Inside the function
            spec = 0;
            if (commaned == "fun"){//closing the function
                end=0;
                functions.push_back(funcs);
                string nameOfFunc;
                //can be named as one of the original functions and mess up slightly
                cout<<"Enter 3 character function name, should be different than the original functions: ";
                std::getline(std::cin, nameOfFunc);
                functionen.push_back(nameOfFunc);
                funcs = "";
                //000111001001101111100011100000con000con011
                //000 111 001 001 101 111 100 011 100 000 con 000 con 011
                //mov con  1   >   5  con  4  !=   4  mov end mov end pri
                //000111001001101111100010100000con000con011
                //000 111 001 001 101 111 100 010 100 000 con 000 con 011
                //mov con  1   >   5  con  4   =   4  mov end mov end pri
                //000111001000101111100011100000con000con011
                //000 111 001 000 101 111 100 011 100 000 con 000 con 011
                //mov con  1   <   5  con  4  !=   4  mov end mov end pri
                //000111001000101111100010100000con000con011
                //000 111 001 000 101 111 100 010 100 000 con 000 con 011
                //mov con  1   <   5  con  4   =   4  mov end mov end pri
            }else if(commaned == "con" && isConditional){//closing the conditional
                if (nests-1 ==0){
                    end=0;
                    isConditional = false;
                    if (isConditionalen != true){
                        string nameOfFunc;
                        cout<<"Enter 3 character conditional name, should be different than the original functions: ";
                        std::getline(std::cin, nameOfFunc);
                        functionen.push_back(nameOfFunc);
                        functions.push_back("111" + funcs + "con");
                    }
                    nextCommand(CondEval(funcs), false);
                    funcs = "";
                    nests = 0;
                }else{
                    funcs += commaned;
                    nests -= 1;
                }
            }else{//lines of the function
                if(commaned == "111") nests += 1;
                funcs += commaned;    
            }
        }else{//not inside function
            cout << "Command: " << commaned << endl << input << "  " << ii << "  " <<  input.substr(ii*3, 3) << endl;
            if(commaned == "111"){
                end= 1;
                nests += 1;
                isConditional = true;
                cout << ii << endl;
            }else if(commaned == "fun"){//opening the function
                end = 1;
            }else{//calling a function/plain
                size_t iters = 0;
                bool found = false;
                for (string funky: functionen){//Calling a new function
                    if (commaned == funky){
                        found = true;
                        string fullCommand= functions[iters];
                        nextCommand(fullCommand, true);
                        //executes the new function
                        break;
                    }
                    iters ++;
                }
                if (found == false){//plain--Spec functions && (commaned != "001" && commaned != "010")
                    checkCommand(commaned);
                }
            }
        }
        funcSpec = commaned;
    }
    return input;
}
string checkCommand(string cmd){
    string input = "";
    if (cmd == "000"){
        move();    
    }
    if (cmd == "011"){
        giveInfo();
    }
    if (cmd == "100"){
        getBall();
    }
    if (cmd=="101"){
        placeBall();    
    }
    if (cmd == "110"){
        autoGetBall();
    }
    if (cmd == "cmd"){
        cout << "current functions:  ";
        for (string funky : functionen){
            cout << funky<<" , ";    
        }
        cout << endl;
    }
    if (cmd == "111" && lastCommand!="111"){
        checkCommand(lastCommand);
    }
    lastCommand = cmd;
    return input;
}
//A B C D E F G H I
//0 1 2 3 4 5 6 7 8
int main()
{
    cout << "Instructions:"<<endl<< "000 to move"<< endl<<"001 to turn--include 4 digit base-19 input between any 2 characters"<<endl<<"010 to set speed--include 4 digit binary input between any 2 characters"<<endl<<"011 to display current status"<< endl<< "100 to get ball"<<endl<<"101 to place ball"<<endl<< "110 to auto get ball"<<endl<<endl<< "Turning is a 4 digit nonadecimal or base-19 code. The first 2 digits are turning in the xz plane,\n the second 2 digits are for turning in the xy plane A(10) B C D E F G H I(18)"<<endl<<endl<< "Use ..fun.. to create a command then enter the name for the command.\nReminder: don't call the command you created in the command\n";
    cout << "First 2 digits for turning are for the xz direction, the next two are for xy direction:\n 90 degrees is 4E, 180 is 99, 270 is E4, 360 is II(max 19^2-1)"<<endl<<endl<< "For conditionals, use 111 to start and con to end the conditional, and use 3 digit binary inputs for the index, comparison operator, and value to compare against\nThe comparison operators are: < is 000, > is 001, == is 010, != is 011"<<endl<<"NEVER USE 111 unless for conditionals as you need to have equal numbers of 111 and con"<<endl;
    giveInfo();
    string line;
    while (true){
        cout << "Enter next command: ";
        line = nextCommand("1", false);
    }
    return 0;
}
//End of main.cpp