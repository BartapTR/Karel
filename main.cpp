#include <iostream>
#include <vector>
#include <array>
#include <cmath>
#include <string>
#include <cctype>
#include <algorithm>
#include <unordered_map>
#include <sstream>

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

std::unordered_map<string, string> functionMap;// merge functionen and functions into a single map for easier lookup, first is the name

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

//"we're using std::pow now"

int convFromBinary(string inphut){
    int numbir = 0;
    for(int i=0;i<inphut.length();i++){
        if (inphut[i]=='0'){
            continue;    
        }
        numbir += (inphut[i]-'0')*std::pow(2, inphut.length()-i-1);
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
    cout << "Distance to ball: " << distance << '\n';
    // A negative distance is the "no ball" sentinel. Check it before using
    // the returned index, because an empty vector has no valid element to erase.
    if (distance >= 0 && distance <= 5){
        cout << " Got Ball"<<'\n';
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
        cout << "No balls available to collect." << '\n';
        return -1;
    }

    string line;
    cout << "How many times to move to get to ball: ";
    std::getline(std::cin,line);
    const int steps = stoi(line);
    if (steps <= 0){
        cout << "Step count must be > 0" << '\n';
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
        cout << "Already at the closest ball." << '\n';
        return 0;
    }
    
    const float requiredSpeed = hypotenuse/steps;
    int requiredTurn;
    
    cout << '\n' << "Dy, Dx, Dz " << deltaY << ", " << deltaX << ", " << deltaZ;
    
    if (deltaY<0){//
        int degree = std::asin(-deltaY/hypotenuse)*180/PI;
        cout << '\n' <<"degree: "<<degree<<"  ";
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
        cout << '\n' << "To get to the closest ball, set speed to " << requiredSpeed << " and turn " << requiredTurn<<" 'it's "<<requiredTurn/19<<'\n';
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
        cout <<'\n'<< "Caution: Hit Wall X"<< "\n\n";
    }
    if (std::abs(t2)>=radius){
        t2 = radius*t2/std::abs(t2)-0.01f;
        cout <<'\n'<<"Caution: Hit Wall Y"<< "\n\n";
    }
    if (std::abs(t3)>=radius){
        t3 = radius*t3/std::abs(t3)-0.01f;
        cout <<'\n'<<"Caution: Hit Wall Z"<< "\n\n";
        
    }
    position[0] = t1;
    position[1] = t2;
    position[2] = t3;
}
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
    num = convFromBinary(newSpeed);
    normalMovement = num;
    cout << "Set speed to " << normalMovement << '\n';
}
float getDistance();
void giveInfo(){
    cout << '\n' << "You are at: "<<position[0]<<", "<<position[1]<<", "<<position[2]<< '\n'<< "with a x/y-direction: "<< xydirection<< " Degrees"<< '\n'<< "with a x/z-direction: "<< xzdirection<< " Degrees"<< '\n' << "Speed: " << normalMovement<< '\n' << "Distance from origin: " << 0.01*round(100*getDistance());
    // This display only needs the distance, so it reads that field from the
    // same result type used by getBall and autoGetBall.
    cout << '\n' << "Distance to closest Ball: " << getClosestBallInfo()[DISTANCE_INDEX]<<'\n'<<"Number of balls: "<<balls.size()<<'\n'<< '\n';
}
bool checkCommand(string cmd);
float getDistance(){
    float n1 = position[0];
    float n2 = position[1];
    float n3 = position[2];
    const vector<float> vecco = {n1,n2,n3};
    return RMS(vecco);
}

bool checkCond (string cond){
	//const int Indexes[] = {normalMovement, xydirection, getDistance(), balls.size(), getClosestBallInfo()[DISTANCE_INDEX], position[0], position[1], position[2]};
	const int Index = convFromBinary(cond.substr(0, 8));//No need for a list unless checking variable values
	int comparison = convFromBinary(cond.substr(8, 8));
	const int value = convFromBinary(cond.substr(16, 8));

    if (comparison > 3) comparison = 3;

    bool comps[] = {Index < value, Index > value, Index == value, Index != value};
    cout <<  Index << "  "<< comparison << "  "<< value << " is " << comps[comparison] << '\n';

	return comps[comparison];
}

string CondEval (string statement){
	bool IsCond = true;
	string nonCondStatement;
	string condition = "";
	string newCond = "";
	vector<bool> isTrue;
	int isNested = 0;
    int paraCount = 0;
	vector<string> operations;

    cout << "Full conditional statement: " << statement << '\n';
	for (int i = 0; i < statement.length()/8; i++){
		const string Cmd = statement.substr(8*i, 8);
        //For nested conditionals
        if (newCond == "b") newCond = "";

        if (Cmd == "10001000" && IsCond == false){///IF only if it's not right after the conditional statement
            newCond = "";
			isNested++;
		}
        if (Cmd == "00001001" && IsCond == false && isNested > 0){//(
			paraCount++;
		}
		if (Cmd == "00001010"/* ) */&& IsCond == false && isNested > 0){//because this isn't exclusive to conditionals, it requires an extra check
            if(paraCount>0) paraCount--;//so make isNested to ParaCount
            if (paraCount == 0) {
                nonCondStatement += CondEval(newCond);
                newCond = "b";
                isNested--;
            }
		}

        //For the main conditional statement
		if (IsCond){             //if((i+1)%4==1)//Index	if((i+1)%4==2)//comparison op	if((i+1)%4==3)//Value to compare against
			if((i+1)%4==0){      //Comparison operator between conditions
				if(Cmd != "00000100" && Cmd != "00000101"){//not OR or AND
                    if(Cmd != "10001000") {//fixes a problem where the nested conditional is right after the condition of the parent conditional
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
        //both have safety againgst nested conditionals
		}else if(isNested > 0){//You need to count nests to keep track of the paranthesis
			if(Cmd != "10001000" && (Cmd != "00001001" || isNested < paraCount)) newCond += Cmd;
        }else{
           if(newCond == "" && isNested == 0) nonCondStatement += Cmd;
        }
        cout << "Command: " << Cmd << "  NonCond: " << nonCondStatement  << "  condition: " << condition << "  newCond: " << newCond << "  isNested: " << isNested << "  paraCount: " << paraCount << endl;
    }
    vector<bool> newIsTrue = isTrue;
	if (operations.size()>0){           //–Repeatedly Check/change cont if isTrue[i] is operations[i] with isTrue[i+1] 
		vector<string> newOperations = operations;
		while (newIsTrue.size() != 1){
            cout << "iteration" << '\n';
			bool second= newIsTrue[1];
			string oper = newOperations[0];
            newIsTrue[1] = newIsTrue.back();
            newIsTrue.pop_back();
            newOperations[0] = newOperations.back();
            newOperations.pop_back();
            cout << newIsTrue[0] << "  " << oper << "  " << second << '\n';
			if (oper=="00000100") newIsTrue[0] = newIsTrue[0]==1 || second==1;	
            if (oper=="00000101") newIsTrue[0] = newIsTrue[0]==1 && second==1;
        }
        if (newIsTrue[0] == 0) nonCondStatement = "";
        cout << '\n' << "nonCondStatement: " << nonCondStatement << '\n';
        return nonCondStatement;
	}else{
        if(isTrue[0] == 0) nonCondStatement = "";
        cout << '\n' << "nonCondStatement: " << nonCondStatement << '\n';
        return nonCondStatement;
    }
}

string commandEvaluation (string input, bool nameAble) {//, bool nameAble

    string text;
    if(input.length()<2){
        cout << "Enter next command: ";
        std::getline(std::cin, text);
    }else{
        text = input;   
    }

    std::stringstream ss(text);
    string word;
    vector<string> words;

    // The >> operator automatically skips consecutive whitespace
    while (ss >> word) {
        //word.lenght()/8 allows 16 bit input to be 2 commands instead of being ignored
        size_t wL = word.length()/8;
        for (size_t n = 0 ; n < wL ; n++) {
            string worde = word.substr(8*n,8);
            cout << endl << "cmd " << worde;
            words.push_back(worde);
        }
    }

    //Have a list of commands at this point

    std::unordered_map<string, int> evalMap = 
    {
    {"Para", 0}, {"Nests", 0}, {"ParaFor", 0}, {"Formatting", 0}, {"howManyMore", 0}, {"whatIsExpected", 0}
    //mainPara   for condition  func/condition  ignore 1st/2nd      setspeed or turn    format, value paranthesis
    };

    string MainParaStatement = "";

    for (string cmd: words) {
        if (cmd.length() != 8) continue;//might be redundant
        int code = convFromBinary(cmd);
        //first check normal commands, about 10 of them also means evalmap is

        if (code == 10) evalMap["Para"]--;
        if (code == 9) evalMap["Para"]++;

        cout << "Main: " << MainParaStatement << ".  " << code << "  " << evalMap["Para"] << "  " << evalMap["Nests"] << "  " << evalMap["ParaFor"] << "  " << evalMap["Formatting"] << "  " << evalMap["howManyMore"] << "  " << evalMap["whatIsExpected"] << endl;

        if(evalMap["Para"] == 0){
            if (evalMap["ParaFor"] == 135) {//If it's a function just add to main statement
                evalMap["ParaFor"] = 0;
                string nameOfFunc;
                //can be named as one of the original functions and mess up slightly
                cout<<"Enter 8-bit function code, should be different than the original functions: ";
                std::getline(std::cin, nameOfFunc);
                functionMap[nameOfFunc] = MainParaStatement; // Store the function in the map
                cout << "Main" << MainParaStatement << endl;
                MainParaStatement = "";
            }else if (evalMap["ParaFor"] == 136) {//conditional is the main thing
                evalMap["ParaFor"] = 0;
                //can be named as one of the original functions and mess up slightly
                if (nameAble){
                    string nameOfFunc;
                    cout<<"Enter 8-bit conditional code, should be different than the original functions: ";
                    std::getline(std::cin, nameOfFunc);
                    functionMap[nameOfFunc] = "10001000 00001001" + MainParaStatement + "00001010"; // Store the conditional in the map with "conditional, (, and )" so that it gets evaluated as a conditional
                }
                commandEvaluation(CondEval(MainParaStatement), false);//condeval returns only the non-conditional part (when we give the conditional + non-cond part) that we evaluate again here
                MainParaStatement = "";
            }
        }else{
            //Fixed problem that didn't allow nested-ifs because of missing paranthesis
            if ((code != 9 && code != 10) || evalMap["Para"] > 1 || (evalMap["Para"] == 1 && code == 10)) MainParaStatement += cmd;
        }

        if (evalMap["ParaFor"] != 0 && evalMap["ParaFor"] != 135  && evalMap["ParaFor"] != 136) {//speed, turn, or other function that requires a parameter
            //function paranthesis value paranthesis
            if (evalMap["whatIsExpected"] == 2){
                evalMap["whatIsExpected"] = 0;//reset after actual value
                cout << "Main statement: " << MainParaStatement << endl;
                if (evalMap["ParaFor"] == 129){
                    MainParaStatement = MainParaStatement.substr(2, 4);
                    cout << endl << MainParaStatement << endl;
                    turn(MainParaStatement);
                }if (evalMap["ParaFor"] == 130){
                    setSpeed(MainParaStatement);
                }
                evalMap["ParaFor"] = 0;
            }else{
                MainParaStatement = cmd;//set value 2 times but only the actual value is registered
                evalMap["whatIsExpected"] ++;
            }
        }else if(evalMap["ParaFor"] == 0){
            if (checkCommand(cmd) == false){//Execute regular command, else check whether it's a user-function (that requires paramaters)
                if(cmd[0] == '1') evalMap["ParaFor"] = code;// bit checking for parameter requirement
                
                if(functionMap.contains(cmd)){//what do you use for the parameter if it wants that,?
                    cout << functionMap[cmd];    
                    commandEvaluation(functionMap[cmd], false);
                }
            }
        }
    }

    return input;
}
//10001000 00001001 00000001 00000010 00000001 10001000 00001001 00000010 00000000 00000101 00000000 00001010 00001010
//if(1==1){if(2<5){move}}
//10000111 00001001 10001000 00001001 00000001 00000010 00000001 10001000 00001001 00000010 00000000 00000101 00000000 00001010 00001010 00000011 00001010
//function {if(1==1){if(2<5){move}} print}
bool checkCommand(string cmd){
    bool output = false;
    if (cmd == "00000000"){
        output = true;
        move();    
    }if (cmd == "00000011"){
        output = true;
        giveInfo();
    }if (cmd == "00000100"){
        output = true;
        getBall();
    }if (cmd=="00000101"){
        output = true;
        placeBall();    
    }if (cmd == "00000110"){
        output = true;
        autoGetBall();
    }if (cmd == "cmd"){
        output = true;
        cout << "Current functions:  ";
        for (const auto& pair: functionMap){
            cout << pair.first<<" ; ";    
        }
        cout << '\n';
    }
    lastCommand = cmd;
    return output;
}
//A B C D E F G H I
//0 1 2 3 4 5 6 7 8
int main()
{
    cout << "Instructions:"<<'\n'<< "000 to move"<< '\n'<<"001 to turn--include 4 digit base-19 input between any 2 characters"<<'\n'<<"010 to set speed--include 4 digit binary input between any 2 characters"<<'\n'<<"011 to display current status"<< '\n'<< "100 to get ball"<<'\n'<<"101 to place ball"<<'\n'<< "110 to auto get ball"<< "\n\n"<< "Turning is a 4 digit nonadecimal or base-19 code. The first 2 digits are turning in the xz plane,\n the second 2 digits are for turning in the xy plane A(10) B C D E F G H I(18)"<< "\n\n"<< "Use ..fun.. to create a command then enter the name for the command.\nReminder: don't call the command you created in the command\n";
    cout << "First 2 digits for turning are for the xz direction, the next two are for xy direction:\n 90 degrees is 4E, 180 is 99, 270 is E4, 360 is II(max 19^2-1)"<< "\n\n"<< "For conditionals, use 111 to start and con to end the conditional, and use 3 digit binary inputs for the index, comparison operator, and value to compare against\nThe comparison operators are: < is 000, > is 001, == is 010, != is 011"<<'\n'<<"NEVER USE 111 unless for conditionals as you need to have equal numbers of 111 and con"<<'\n';
    giveInfo();
    while (true){
        commandEvaluation("1", true);
    }
    return 0;
}
//End of main.cpp